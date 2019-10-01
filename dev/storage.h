#pragma once

#include <memory>   //  std::unique/shared_ptr, std::make_unique/shared
#include <string>   //  std::string
#include <sqlite3.h>
#include <type_traits>  //  std::remove_reference, std::is_base_of, std::decay, std::false_type, std::true_type
#include <cstddef>  //  std::ptrdiff_t
#include <iterator> //  std::input_iterator_tag, std::iterator_traits, std::distance
#include <functional>   //  std::function
#include <sstream>  //  std::stringstream
#include <map>  //  std::map
#include <vector>   //  std::vector
#include <tuple>    //  std::tuple_size, std::tuple, std::make_tuple
#include <utility>  //  std::forward, std::pair
#include <set>  //  std::set
#include <algorithm>    //  std::find

#include "alias.h"
#include "row_extractor.h"
#include "error_code.h"
#include "type_printer.h"
#include "tuple_helper.h"
#include "constraints.h"
#include "table_type.h"
#include "type_is_nullable.h"
#include "field_printer.h"
#include "rowid.h"
#include "aggregate_functions.h"
#include "operators.h"
#include "select_constraints.h"
#include "core_functions.h"
#include "conditions.h"
#include "statement_binder.h"
#include "column_result.h"
#include "mapped_type_proxy.h"
#include "sync_schema_result.h"
#include "table_info.h"
#include "storage_impl.h"
#include "journal_mode.h"
#include "field_value_holder.h"
#include "view.h"
#include "ast_iterator.h"
#include "storage_base.h"
#include "prepared_statement.h"

namespace sqlite_orm {
    
    namespace conditions {
        
        template<class S>
        struct dynamic_order_by_t;
    }
    
    namespace internal {
        
        /**
         *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage` function.
         */
        template<class ...Ts>
        struct storage_t : storage_base {
            using self = storage_t<Ts...>;
            using impl_type = storage_impl<Ts...>;
            using storage_base::serialize_column_schema;
            
            /**
             *  @param filename database filename.
             *  @param impl_ storage_impl head
             */
            storage_t(const std::string &filename, impl_type impl_):
            storage_base{filename, foreign_keys_count(impl_)},
            impl(std::move(impl_))
            {}
            
            storage_t(const storage_t &other):
            storage_base(other),
            impl(other.impl)
            {}
            
        protected:
            impl_type impl;
            
            template<class T, class S, class ...Args>
            friend struct view_t;
            
            template<class S>
            friend struct conditions::dynamic_order_by_t;
            
            template<class V>
            friend struct iterator_t;
            
            template<class ...Cs>
            std::string serialize_column_schema(const constraints::primary_key_t<Cs...> &fk) {
                std::stringstream ss;
                ss << static_cast<std::string>(fk) << " (";
                std::vector<std::string> columnNames;
                columnNames.reserve(std::tuple_size<decltype(fk.columns)>::value);
                iterate_tuple(fk.columns, [&columnNames, this](auto &c){
                    columnNames.push_back(this->impl.column_name(c));
                });
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << ") ";
                return ss.str();
            }
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            template<class ...Cs, class ...Rs>
            std::string serialize_column_schema(const constraints::foreign_key_t<std::tuple<Cs...>, std::tuple<Rs...>> &fk) {
                std::stringstream ss;
                std::vector<std::string> columnNames;
                using columns_type_t = typename std::decay<decltype(fk)>::type::columns_type;
                constexpr const size_t columnsCount = std::tuple_size<columns_type_t>::value;
                columnNames.reserve(columnsCount);
                iterate_tuple(fk.columns, [&columnNames, this](auto &v){
                    columnNames.push_back(this->impl.column_name(v));
                });
                ss << "FOREIGN KEY( ";
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1){
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << ") REFERENCES ";
                std::vector<std::string> referencesNames;
                using references_type_t = typename std::decay<decltype(fk)>::type::references_type;
                constexpr const size_t referencesCount = std::tuple_size<references_type_t>::value;
                referencesNames.reserve(referencesCount);
                {
                    using first_reference_t = typename std::tuple_element<0, references_type_t>::type;
                    using first_reference_mapped_type = typename internal::table_type<first_reference_t>::type;
                    auto refTableName = this->impl.template find_table_name<first_reference_mapped_type>();
                    ss << refTableName << " ";
                }
                iterate_tuple(fk.references, [&referencesNames, this](auto &v){
                    referencesNames.push_back(this->impl.column_name(v));
                });
                ss << "( ";
                for(size_t i = 0; i < referencesNames.size(); ++i){
                    ss << referencesNames[i];
                    if(i < referencesNames.size() - 1){
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << ") ";
                if(fk.on_update){
                    ss << static_cast<std::string>(fk.on_update) << " " << fk.on_update._action << " ";
                }
                if(fk.on_delete){
                    ss << static_cast<std::string>(fk.on_delete) << " " << fk.on_delete._action << " ";
                }
                return ss.str();
            }
#endif
            
            template<class I>
            void create_table(sqlite3 *db, const std::string &tableName, I *impl) {
                std::stringstream ss;
                ss << "CREATE TABLE '" << tableName << "' ( ";
                auto columnsCount = impl->table.columns_count;
                auto index = 0;
                impl->table.for_each_column_with_constraints([columnsCount, &index, &ss, this] (auto &c) {
                    ss << this->serialize_column_schema(c);
                    if(index < columnsCount - 1) {
                        ss << ", ";
                    }
                    index++;
                });
                ss << ") ";
                if(impl->table._without_rowid) {
                    ss << "WITHOUT ROWID ";
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class I>
            void backup_table(sqlite3 *db, I *impl) {
                
                //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
                //  a name already exists we append suffix 1, then 2, etc until we find a free name..
                auto backupTableName = impl->table.name + "_backup";
                if(impl->table_exists(backupTableName, db)){
                    int suffix = 1;
                    do{
                        std::stringstream stream;
                        stream << suffix;
                        auto anotherBackupTableName = backupTableName + stream.str();
                        if(!impl->table_exists(anotherBackupTableName, db)){
                            backupTableName = anotherBackupTableName;
                            break;
                        }
                        ++suffix;
                    }while(true);
                }
                
                this->create_table(db, backupTableName, impl);
                
                impl->copy_table(db, backupTableName);
                
                this->drop_table_internal(impl->table.name, db);
                
                impl->rename_table(db, backupTableName, impl->table.name);
            }
            
            template<class O>
            void assert_mapped_type() const {
                using mapped_types_tuples = std::tuple<typename Ts::object_type...>;
                static_assert(tuple_helper::has_type<O, mapped_types_tuples>::value, "type is not mapped to a storage");
            }
            
            template<class O>
            auto& get_impl() const {
                return this->impl.template get_impl<O>();
            }
            
            template<class T>
            typename std::enable_if<is_bindable<T>::value, std::string>::type string_from_expression(const T &, bool) const {
                return "?";
            }
            
            std::string string_from_expression(std::nullptr_t, bool /*noTableName*/) const {
                return "?";
            }
            
            template<class T>
            std::string string_from_expression(const alias_holder<T> &, bool /*noTableName*/) const {
                return T::get();
            }
            
            template<class R, class S, class ...Args>
            std::string string_from_expression(const core_functions::core_function_t<R, S, Args...> &c, bool noTableName) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << "(";
                std::vector<std::string> args;
                using args_type = typename std::decay<decltype(c)>::type::args_type;
                args.reserve(std::tuple_size<args_type>::value);
                iterate_tuple(c.args, [&args, this, noTableName](auto &v){
                    args.push_back(this->string_from_expression(v, noTableName));
                });
                for(size_t i = 0; i < args.size(); ++i){
                    ss << args[i];
                    if(i < args.size() - 1){
                        ss << ", ";
                    }
                }
                ss << ")";
                return ss.str();
            }
            
            template<class T, class E>
            std::string string_from_expression(const as_t<T, E> &als, bool noTableName) const {
                auto tableAliasString = alias_extractor<T>::get();
                return this->string_from_expression(als.expression, noTableName) + " AS " + tableAliasString;
            }
            
            template<class T, class C>
            std::string string_from_expression(const alias_column_t<T, C> &als, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << T::get() << "'.";
                }
                ss << this->string_from_expression(als.column, true);
                return ss.str();
            }
            
            std::string string_from_expression(const std::string &, bool /*noTableName*/) const {
                return "?";
            }
            
            std::string string_from_expression(const char *, bool /*noTableName*/) const {
                return "?";
            }
            
            template<class F, class O>
            std::string string_from_expression(F O::*m, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << "\"" << this->impl.column_name(m) << "\"";
                return ss.str();
            }
            
            std::string string_from_expression(const rowid_t &rid, bool /*noTableName*/) const {
                return static_cast<std::string>(rid);
            }
            
            std::string string_from_expression(const oid_t &rid, bool /*noTableName*/) const {
                return static_cast<std::string>(rid);
            }
            
            std::string string_from_expression(const _rowid_t &rid, bool /*noTableName*/) const {
                return static_cast<std::string>(rid);
            }
            
            template<class O>
            std::string string_from_expression(const table_rowid_t<O> &rid, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << static_cast<std::string>(rid);
                return ss.str();
            }
            
            template<class O>
            std::string string_from_expression(const table_oid_t<O> &rid, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << static_cast<std::string>(rid);
                return ss.str();
            }
            
            template<class O>
            std::string string_from_expression(const table__rowid_t<O> &rid, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << static_cast<std::string>(rid);
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::group_concat_double_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                auto expr2 = this->string_from_expression(f.y, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::group_concat_single_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class L, class R, class ...Ds>
            std::string string_from_expression(const binary_operator<L, R, Ds...> &f, bool noTableName) const {
                std::stringstream ss;
                auto lhs = this->string_from_expression(f.lhs, noTableName);
                auto rhs = this->string_from_expression(f.rhs, noTableName);
                ss << "(" << lhs << " " << static_cast<std::string>(f) << " " << rhs << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::min_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::max_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::total_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::sum_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::count_asterisk_t<T> &, bool noTableName) const {
                return this->string_from_expression(aggregate_functions::count_asterisk_without_type{}, noTableName);
            }
            
            std::string string_from_expression(const aggregate_functions::count_asterisk_without_type &f, bool /*noTableName*/) const {
                std::stringstream ss;
                ss << static_cast<std::string>(f) << "(*)";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::count_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const aggregate_functions::avg_t<T> &a, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(a.t, noTableName);
                ss << static_cast<std::string>(a) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const distinct_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const all_t<T> &f, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t, noTableName);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T, class F>
            std::string string_from_expression(const column_pointer<T, F> &c, bool noTableName) const {
                std::stringstream ss;
                if(!noTableName){
                    ss << "'" << this->impl.template find_table_name<T>() << "'.";
                }
                auto &impl = this->get_impl<T>();
                ss << "\"" << impl.column_name_simple(c.field) << "\"";
                return ss.str();
            }
            
            template<class T>
            std::vector<std::string> get_column_names(const T &t) const {
                auto columnName = this->string_from_expression(t, false);
                if(columnName.length()){
                    return {columnName};
                }else{
                    throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                }
            }
            
            template<class T>
            std::vector<std::string> get_column_names(const internal::asterisk_t<T> &) const {
                std::vector<std::string> res;
                res.push_back("*");
                return res;
            }
            
            template<class ...Args>
            std::vector<std::string> get_column_names(const internal::columns_t<Args...> &cols) const {
                std::vector<std::string> columnNames;
                columnNames.reserve(static_cast<size_t>(cols.count));
                iterate_tuple(cols.columns, [&columnNames, this](auto &m){
                    auto columnName = this->string_from_expression(m, false);
                    if(columnName.length()){
                        columnNames.push_back(columnName);
                    }else{
                        throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                    }
                });
                return columnNames;
            }
            
            /**
             *  Takes select_t object and returns SELECT query string
             */
            template<class T, class ...Args>
            std::string string_from_expression(const internal::select_t<T, Args...> &sel, bool /*noTableName*/) const {
                std::stringstream ss;
                if(!is_base_of_template<T, compound_operator>::value){
                    if(!sel.highest_level){
                        ss << "( ";
                    }
                    ss << "SELECT ";
                }
                if(get_distinct(sel.col)) {
                    ss << static_cast<std::string>(distinct(0)) << " ";
                }
                auto columnNames = this->get_column_names(sel.col);
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                auto tableNamesSet = this->parse_table_name(sel.col);
                internal::join_iterator<Args...>()([&tableNamesSet, this](const auto &c){
                    using original_join_type = typename std::decay<decltype(c)>::type::join_type::type;
                    using cross_join_type = typename internal::mapped_type_proxy<original_join_type>::type;
                    auto crossJoinedTableName = this->impl.template find_table_name<cross_join_type>();
                    auto tableAliasString = alias_extractor<original_join_type>::get();
                    std::pair<std::string, std::string> tableNameWithAlias(std::move(crossJoinedTableName), std::move(tableAliasString));
                    tableNamesSet.erase(tableNameWithAlias);
                });
                if(!tableNamesSet.empty()){
                    ss << "FROM ";
                    std::vector<std::pair<std::string, std::string>> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                    for(size_t i = 0; i < tableNames.size(); ++i) {
                        auto &tableNamePair = tableNames[i];
                        ss << "'" << tableNamePair.first << "' ";
                        if(!tableNamePair.second.empty()){
                            ss << tableNamePair.second << " ";
                        }
                        if(int(i) < int(tableNames.size()) - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                }
                iterate_tuple(sel.conditions, [&ss, this](auto &v){
                    this->process_single_condition(ss, v);
                });
                if(!is_base_of_template<T, compound_operator>::value){
                    if(!sel.highest_level){
                        ss << ") ";
                    }
                }
                return ss.str();
            }
            
            template<class T, class ...Args>
            std::string string_from_expression(const get_all_t<T, Args...> &get, bool /*noTableName*/) const {
                std::stringstream ss;
                ss << "SELECT ";
                auto &impl = this->get_impl<T>();
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss
                    << "\"" << impl.table.name << "\"."
                    << "\""
                    << columnNames[i]
                    << "\""
                    ;
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << "FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, get.conditions);
                return ss.str();
            }
            
            template<class ...Args, class ...Wargs>
            std::string string_from_expression(const update_all_t<set_t<Args...>, Wargs...> &upd, bool /*noTableName*/) const {
                std::stringstream ss;
                ss << "UPDATE ";
                std::set<std::pair<std::string, std::string>> tableNamesSet;
                upd.set.for_each([this, &tableNamesSet](auto &asgn) {
                    auto tableName = this->parse_table_name(asgn.lhs);
                    tableNamesSet.insert(tableName.begin(), tableName.end());
                });
                if(!tableNamesSet.empty()){
                    if(tableNamesSet.size() == 1){
                        ss << " '" << tableNamesSet.begin()->first << "' ";
                        ss << static_cast<std::string>(upd.set) << " ";
                        std::vector<std::string> setPairs;
                        upd.set.for_each([this, &setPairs](auto &asgn){
                            std::stringstream sss;
                            sss << this->string_from_expression(asgn.lhs, true);
                            sss << " " << static_cast<std::string>(asgn) << " ";
                            sss << this->string_from_expression(asgn.rhs, false) << " ";
                            setPairs.push_back(sss.str());
                        });
                        auto setPairsCount = setPairs.size();
                        for(size_t i = 0; i < setPairsCount; ++i) {
                            ss << setPairs[i] << " ";
                            if(i < setPairsCount - 1) {
                                ss << ", ";
                            }
                        }
                        this->process_conditions(ss, upd.conditions);
                        return ss.str();
                    }else{
                        throw std::system_error(std::make_error_code(orm_error_code::too_many_tables_specified));
                    }
                }else{
                    throw std::system_error(std::make_error_code(orm_error_code::incorrect_set_fields_specified));
                }
            }
            
            template<class T, class ...Args>
            std::string string_from_expression(const remove_all_t<T, Args...> &rem, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "DELETE FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, rem.conditions);
                return ss.str();
            }
            
            template<class T, class ...Ids>
            std::string string_from_expression(const get_t<T, Ids...> &g, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "SELECT ";
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << "FROM '" << impl.table.name << "' WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                if(!primaryKeyColumnNames.empty()){
                    for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                        ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                        if(i < primaryKeyColumnNames.size() - 1) {
                            ss << "AND ";
                        }
                        ss << ' ';
                    }
                    return ss.str();
                }else{
                    throw std::system_error(std::make_error_code(orm_error_code::table_has_no_primary_key_column));
                }
            }
            
            template<class T, class ...Ids>
            std::string string_from_expression(const get_pointer_t<T, Ids...> &g, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "SELECT ";
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << "FROM '" << impl.table.name << "' WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                if(!primaryKeyColumnNames.empty()){
                    for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                        ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                        if(i < primaryKeyColumnNames.size() - 1) {
                            ss << "AND ";
                        }
                        ss << ' ';
                    }
                    return ss.str();
                }else{
                    throw std::system_error(std::make_error_code(orm_error_code::table_has_no_primary_key_column));
                }
            }
            
            template<class T, bool by_ref>
            std::string string_from_expression(const update_t<T, by_ref> &upd, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "UPDATE '" << impl.table.name << "' SET ";
                std::vector<std::string> setColumnNames;
                impl.table.for_each_column([&setColumnNames](auto &c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        setColumnNames.emplace_back(c.name);
                    }
                });
                for(size_t i = 0; i < setColumnNames.size(); ++i) {
                    ss << "\"" << setColumnNames[i] << "\"" << " = ?";
                    if(i < setColumnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << "WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " AND";
                    }
                    ss << " ";
                }
                return ss.str();
            }
            
            template<class T, class ...Ids>
            std::string string_from_expression(const remove_t<T, Ids...> &rem, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "DELETE FROM '" << impl.table.name << "' ";
                ss << "WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << "AND ";
                    }
                }
                return ss.str();
            }
            
            template<class T, bool by_ref, class ...Cols>
            std::string string_from_expression(const insert_explicit<T, by_ref, Cols...> &ins, bool /*noTableName*/) const {
                constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                static_assert(colsCount > 0, "Use insert or replace with 1 argument instead");
                this->assert_mapped_type<T>();
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "INSERT INTO '" << impl.table.name << "' ";
                std::vector<std::string> columnNames;
                columnNames.reserve(colsCount);
                iterate_tuple(ins.columns.columns, [&columnNames, this](auto &m){
                    auto columnName = this->string_from_expression(m, true);
                    if(!columnName.empty()){
                        columnNames.push_back(columnName);
                    }else{
                        throw std::system_error(std::make_error_code(orm_error_code::column_not_found));
                    }
                });
                ss << "(";
                for(size_t i = 0; i < columnNames.size(); ++i){
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1){
                        ss << ",";
                    }else{
                        ss << ")";
                    }
                    ss << " ";
                }
                ss << "VALUES (";
                for(size_t i = 0; i < columnNames.size(); ++i){
                    ss << "?";
                    if(i < columnNames.size() - 1){
                        ss << ",";
                    }else{
                        ss << ")";
                    }
                    ss << " ";
                }
                return ss.str();
            }
            
            template<class T, bool by_ref>
            std::string string_from_expression(const insert_t<T, by_ref> &ins, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "INSERT INTO '" << impl.table.name << "' ";
                std::vector<std::string> columnNames;
                auto compositeKeyColumnNames = impl.table.composite_key_columns_names();
                
                impl.table.for_each_column([&impl, &columnNames, &compositeKeyColumnNames] (auto &c) {
                    if(impl.table._without_rowid || !c.template has<constraints::primary_key_t<>>()) {
                        auto it = std::find(compositeKeyColumnNames.begin(),
                                            compositeKeyColumnNames.end(),
                                            c.name);
                        if(it == compositeKeyColumnNames.end()){
                            columnNames.emplace_back(c.name);
                        }
                    }
                });
                
                auto columnNamesCount = columnNames.size();
                if(columnNamesCount){
                    ss << "( ";
                    for(size_t i = 0; i < columnNamesCount; ++i) {
                        ss << "\"" << columnNames[i] << "\"";
                        if(i < columnNamesCount - 1) {
                            ss << ",";
                        }else{
                            ss << ")";
                        }
                        ss << " ";
                    }
                }else{
                    ss << "DEFAULT ";
                }
                ss << "VALUES ";
                if(columnNamesCount){
                    ss << "( ";
                    for(size_t i = 0; i < columnNamesCount; ++i) {
                        ss << "?";
                        if(i < columnNamesCount - 1) {
                            ss << ", ";
                        }else{
                            ss << ")";
                        }
                    }
                }
                return ss.str();
            }
            
            template<class T, bool by_ref>
            std::string string_from_expression(const replace_t<T, by_ref> &rep, bool /*noTableName*/) const {
                auto &impl = this->get_impl<T>();
                std::stringstream ss;
                ss << "REPLACE INTO '" << impl.table.name << "' (";
                auto columnNames = impl.table.column_names();
                auto columnNamesCount = columnNames.size();
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNamesCount - 1) {
                        ss << ",";
                    }else{
                        ss << ")";
                    }
                    ss << " ";
                }
                ss << "VALUES(";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "?";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                return ss.str();
            }
            
            template<class It>
            std::string string_from_expression(const replace_range_t<It> &rep, bool /*noTableName*/) const {
                using expression_type = typename std::decay<decltype(rep)>::type;
                using object_type = typename expression_type::object_type;
                auto &impl = this->get_impl<object_type>();
                std::stringstream ss;
                ss << "REPLACE INTO '" << impl.table.name << "' (";
                auto columnNames = impl.table.column_names();
                auto columnNamesCount = columnNames.size();
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ") ";
                    }
                }
                ss << "VALUES ";
                auto valuesString = [columnNamesCount]{
                    std::stringstream ss;
                    ss << "(";
                    for(size_t i = 0; i < columnNamesCount; ++i) {
                        ss << "?";
                        if(i < columnNamesCount - 1) {
                            ss << ", ";
                        }else{
                            ss << ")";
                        }
                    }
                    return ss.str();
                }();
                auto valuesCount = static_cast<int>(std::distance(rep.range.first, rep.range.second));
                for(auto i = 0; i < valuesCount; ++i) {
                    ss << valuesString;
                    if(i < valuesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                return ss.str();
            }
            
            template<class It>
            std::string string_from_expression(const insert_range_t<It> &ins, bool /*noTableName*/) const {
                using expression_type = typename std::decay<decltype(ins)>::type;
                using object_type = typename expression_type::object_type;
                auto &impl = this->get_impl<object_type>();
                
                std::stringstream ss;
                ss << "INSERT INTO '" << impl.table.name << "' (";
                std::vector<std::string> columnNames;
                impl.table.for_each_column([&columnNames] (auto &c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        columnNames.emplace_back(c.name);
                    }
                });
                
                auto columnNamesCount = columnNames.size();
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNamesCount - 1) {
                        ss << ",";
                    }else{
                        ss << ")";
                    }
                    ss << " ";
                }
                ss << "VALUES ";
                auto valuesString = [columnNamesCount]{
                    std::stringstream ss;
                    ss << "(";
                    for(size_t i = 0; i < columnNamesCount; ++i) {
                        ss << "?";
                        if(i < columnNamesCount - 1) {
                            ss << ", ";
                        }else{
                            ss << ")";
                        }
                    }
                    return ss.str();
                }();
                auto valuesCount = static_cast<int>(std::distance(ins.range.first, ins.range.second));
                for(auto i = 0; i < valuesCount; ++i) {
                    ss << valuesString;
                    if(i < valuesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                return ss.str();
            }
            
            template<class T, class E>
            std::string string_from_expression(const conditions::cast_t<T, E> &c, bool noTableName) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " (";
                ss << this->string_from_expression(c.expression, noTableName) << " AS " << type_printer<T>().print() << ")";
                return ss.str();
            }
            
            template<class T>
            typename std::enable_if<is_base_of_template<T, compound_operator>::value, std::string>::type string_from_expression(const T &op, bool noTableName) const
            {
                std::stringstream ss;
                ss << this->string_from_expression(op.left, noTableName) << " ";
                ss << static_cast<std::string>(op) << " ";
                ss << this->string_from_expression(op.right, noTableName);
                return ss.str();
            }
            
            template<class R, class T, class E, class ...Args>
            std::string string_from_expression(const internal::simple_case_t<R, T, E, Args...> &c, bool noTableName) const {
                std::stringstream ss;
                ss << "CASE ";
                c.case_expression.apply([&ss, this, noTableName](auto &c){
                    ss << this->string_from_expression(c, noTableName) << " ";
                });
                iterate_tuple(c.args, [&ss, this, noTableName](auto &pair){
                    ss << "WHEN " << this->string_from_expression(pair.first, noTableName) << " ";
                    ss << "THEN " << this->string_from_expression(pair.second, noTableName) << " ";
                });
                c.else_expression.apply([&ss, this, noTableName](auto &el){
                    ss << "ELSE " << this->string_from_expression(el, noTableName) << " ";
                });
                ss << "END";
                return ss.str();
            }
             
            template<class T>
            std::string string_from_expression(const conditions::is_null_t<T> &c, bool noTableName) const {
                std::stringstream ss;
                ss << this->string_from_expression(c.t, noTableName) << " " << static_cast<std::string>(c) << " ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const conditions::is_not_null_t<T> &c, bool noTableName) const {
                std::stringstream ss;
                ss << this->string_from_expression(c.t, noTableName) << " " << static_cast<std::string>(c) << " ";
                return ss.str();
            }
            
            template<class C>
            std::string string_from_expression(const conditions::negated_condition_t<C> &c, bool noTableName) const {
                std::stringstream ss;
                ss << static_cast<std::string>(c) << " ";
                auto cString = this->string_from_expression(c.c, noTableName);
                ss << " (" << cString << " ) ";
                return ss.str();
            }
            
            template<class L, class R>
            std::string string_from_expression(const conditions::is_equal_t<L, R> &c, bool noTableName) const {
                auto leftString = this->string_from_expression(c.l, noTableName);
                auto rightString = this->string_from_expression(c.r, noTableName);
                std::stringstream ss;
                ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
                return ss.str();
            }
            
            template<class C>
            typename std::enable_if<is_base_of_template<C, conditions::binary_condition>::value, std::string>::type string_from_expression(const C &c, bool noTableName) const {
                auto leftString = this->string_from_expression(c.l, noTableName);
                auto rightString = this->string_from_expression(c.r, noTableName);
                std::stringstream ss;
                ss << "(" << leftString << " " << static_cast<std::string>(c) << " " << rightString << ")";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const conditions::named_collate<T> &col, bool noTableName) const {
                auto res = this->string_from_expression(col.expr, noTableName);
                return res + " " + static_cast<std::string>(col);
            }
            
            template<class T>
            std::string string_from_expression(const conditions::collate_t<T> &col, bool noTableName) const {
                auto res = this->string_from_expression(col.expr, noTableName);
                return res + " " + static_cast<std::string>(col);
            }
            
            template<class L, class A>
            std::string string_from_expression(const conditions::in_t<L, A> &inCondition, bool noTableName) const {
                std::stringstream ss;
                auto leftString = this->string_from_expression(inCondition.l, noTableName);
                ss << leftString << " " << static_cast<std::string>(inCondition) << " ";
                ss << this->string_from_expression(inCondition.arg, noTableName);
                return ss.str();
            }
            
            template<class L, class E>
            std::string string_from_expression(const conditions::in_t<L, std::vector<E>> &inCondition, bool noTableName) const {
                std::stringstream ss;
                auto leftString = this->string_from_expression(inCondition.l, noTableName);
                ss << leftString << " " << static_cast<std::string>(inCondition) << " ( ";
                for(size_t index = 0; index < inCondition.arg.size(); ++index) {
                    auto &value = inCondition.arg[index];
                    ss << " " << this->string_from_expression(value, noTableName);
                    if(index < inCondition.arg.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " )";
                return ss.str();
            }
            
            template<class A, class T, class E>
            std::string string_from_expression(const conditions::like_t<A, T, E> &l, bool noTableName) const {
                std::stringstream ss;
                ss << this->string_from_expression(l.arg, noTableName) << " ";
                ss << static_cast<std::string>(l) << " ";
                ss << this->string_from_expression(l.pattern, noTableName);
                l.arg3.apply([&ss, this, noTableName](auto &value){
                    ss << " ESCAPE " << this->string_from_expression(value, noTableName);
                });
                return ss.str();
            }
            
            template<class A, class T>
            std::string string_from_expression(const conditions::glob_t<A, T> &l, bool noTableName) const {
                std::stringstream ss;
                ss << this->string_from_expression(l.arg, noTableName) << " ";
                ss << static_cast<std::string>(l) << " ";
                ss << this->string_from_expression(l.pattern, noTableName);
                return ss.str();
            }
            
            template<class A, class T>
            std::string string_from_expression(const conditions::between_t<A, T> &bw, bool noTableName) const {
                std::stringstream ss;
                auto expr = this->string_from_expression(bw.expr, noTableName);
                ss << expr << " " << static_cast<std::string>(bw) << " ";
                ss << this->string_from_expression(bw.b1, noTableName);
                ss << " AND ";
                ss << this->string_from_expression(bw.b2, noTableName);
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(const conditions::exists_t<T> &e, bool noTableName) const {
                std::stringstream ss;
                ss << static_cast<std::string>(e) << " ";
                ss << this->string_from_expression(e.t, noTableName);
                return ss.str();
            }
            
            template<class O>
            std::string process_order_by(const conditions::order_by_t<O> &orderBy) const {
                std::stringstream ss;
                auto columnName = this->string_from_expression(orderBy.o, false);
                ss << columnName << " ";
                if(orderBy._collate_argument.length()){
                    ss << "COLLATE " << orderBy._collate_argument << " ";
                }
                switch(orderBy.asc_desc){
                    case 1:
                        ss << "ASC";
                        break;
                    case -1:
                        ss << "DESC";
                        break;
                }
                return ss.str();
            }
            
            template<class T>
            void process_join_constraint(std::stringstream &ss, const conditions::on_t<T> &t) const {
                ss << static_cast<std::string>(t) << " " << this->string_from_expression(t.arg, false);
            }
            
            template<class F, class O>
            void process_join_constraint(std::stringstream &ss, const conditions::using_t<F, O> &u) const {
                ss << static_cast<std::string>(u) << " (" << this->string_from_expression(u.column, true) << " )";
            }
            
            void process_single_condition(std::stringstream &ss, const conditions::limit_t &limt) const {
                ss << static_cast<std::string>(limt) << " ";
                if(limt.has_offset) {
                    if(limt.offset_is_implicit){
                        ss << limt.off << ", " << limt.lim;
                    }else{
                        ss << limt.lim << " OFFSET " << limt.off;
                    }
                }else{
                    ss << limt.lim;
                }
            }
            
            template<class O>
            void process_single_condition(std::stringstream &ss, const conditions::cross_join_t<O> &c) const {
                ss << static_cast<std::string>(c) << " ";
                ss << " '" << this->impl.template find_table_name<O>() << "'";
            }
            
            template<class O>
            void process_single_condition(std::stringstream &ss, const conditions::natural_join_t<O> &c) const {
                ss << static_cast<std::string>(c) << " ";
                ss << " '" << this->impl.template find_table_name<O>() << "'";
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, const conditions::inner_join_t<T, O> &l) const {
                ss << static_cast<std::string>(l) << " ";
                auto aliasString = alias_extractor<T>::get();
                ss << " '" << this->impl.template find_table_name<typename mapped_type_proxy<T>::type>() << "' ";
                if(aliasString.length()){
                    ss << "'" << aliasString << "' ";
                }
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, const conditions::left_outer_join_t<T, O> &l) const {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, const conditions::left_join_t<T, O> &l) const {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, const conditions::join_t<T, O> &l) const {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class C>
            void process_single_condition(std::stringstream &ss, const conditions::where_t<C> &w) const {
                ss << static_cast<std::string>(w) << " ";
                auto whereString = this->string_from_expression(w.c, false);
                ss << "( " << whereString << ") ";
            }
            
            template<class O>
            void process_single_condition(std::stringstream &ss, const conditions::order_by_t<O> &orderBy) const {
                ss << static_cast<std::string>(orderBy) << " ";
                auto orderByString = this->process_order_by(orderBy);
                ss << orderByString << " ";
            }
            
            template<class ...Args>
            void process_single_condition(std::stringstream &ss, const conditions::multi_order_by_t<Args...> &orderBy) const {
                std::vector<std::string> expressions;
                iterate_tuple(orderBy.args, [&expressions, this](auto &v){
                    auto expression = this->process_order_by(v);
                    expressions.push_back(std::move(expression));
                });
                ss << static_cast<std::string>(orderBy) << " ";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " ";
            }
            
            template<class S>
            void process_single_condition(std::stringstream &ss, const conditions::dynamic_order_by_t<S> &orderBy) const {
                ss << this->storage_base::process_order_by(orderBy) << " ";
            }
            
            template<class ...Args>
            void process_single_condition(std::stringstream &ss, const conditions::group_by_t<Args...> &groupBy) const {
                std::vector<std::string> expressions;
                iterate_tuple(groupBy.args, [&expressions, this](auto &v){
                    auto expression = this->string_from_expression(v, false);
                    expressions.push_back(expression);
                });
                ss << static_cast<std::string>(groupBy) << " ";
                for(size_t i = 0; i < expressions.size(); ++i) {
                    ss << expressions[i];
                    if(i < expressions.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " ";
            }
            
            template<class T>
            void process_single_condition(std::stringstream &ss, const conditions::having_t<T> &hav) const {
                ss << static_cast<std::string>(hav) << " ";
                ss << this->string_from_expression(hav.t, false) << " ";
            }
            
            template<class ...Args>
            void process_conditions(std::stringstream &ss, const std::tuple<Args...> &args) const {
                iterate_tuple(args, [this, &ss](auto &v){
                    this->process_single_condition(ss, v);
                });
            }
            
        public:
            
            template<class T, class ...Args>
            view_t<T, self, Args...> iterate(Args&& ...args) {
                this->assert_mapped_type<T>();
                
                auto con = this->get_connection();
                return {*this, std::move(con), std::forward<Args>(args)...};
            }
            
            template<class O, class ...Args>
            void remove_all(Args&& ...args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::remove_all<O>(std::forward<Args>(args)...));
                this->execute(statement);
            }
            
            /**
             *  Delete routine.
             *  O is an object's type. Must be specified explicitly.
             *  @param ids ids of object to be removed.
             */
            template<class O, class ...Ids>
            void remove(Ids ...ids) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::remove<O>(std::forward<Ids>(ids)...));
                this->execute(statement);
            }
            
            /**
             *  Update routine. Sets all non primary key fields where primary key is equal.
             *  O is an object type. May be not specified explicitly cause it can be deduced by
             *      compiler from first parameter.
             *  @param o object to be updated.
             */
            template<class O>
            void update(const O &o) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::update(o));
                this->execute(statement);
            }
            
            template<class ...Args, class ...Wargs>
            void update_all(internal::set_t<Args...> set, Wargs ...wh) {
                auto statement = this->prepare(sqlite_orm::update_all(std::move(set), std::forward<Wargs>(wh)...));
                this->execute(statement);
            }
            
        protected:
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const T &) const {
                return {};
            }
            
            template<class F, class O>
            std::set<std::pair<std::string, std::string>> parse_table_name(F O::*, std::string alias = {}) const {
                return {std::make_pair(this->impl.template find_table_name<O>(), std::move(alias))};
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::min_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::max_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::sum_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::total_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::group_concat_double_t<T> &f) const {
                auto res = this->parse_table_name(f.t);
                auto secondSet = this->parse_table_name(f.y);
                res.insert(secondSet.begin(), secondSet.end());
                return res;
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::group_concat_single_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::count_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::avg_t<T> &a) const {
                return this->parse_table_name(a.t);
            }
            
            template<class R, class S, class ...Args>
            std::set<std::pair<std::string, std::string>> parse_table_name(const core_functions::core_function_t<R, S, Args...> &f) const {
                std::set<std::pair<std::string, std::string>> res;
                iterate_tuple(f.args, [&res, this](auto &v){
                    auto tableNames = this->parse_table_name(v);
                    res.insert(tableNames.begin(), tableNames.end());
                });
                return res;
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const distinct_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const all_t<T> &f) const {
                return this->parse_table_name(f.t);
            }
            
            template<class L, class R, class ...Ds>
            std::set<std::pair<std::string, std::string>> parse_table_name(const binary_operator<L, R, Ds...> &f) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftSet = this->parse_table_name(f.lhs);
                res.insert(leftSet.begin(), leftSet.end());
                auto rightSet = this->parse_table_name(f.rhs);
                res.insert(rightSet.begin(), rightSet.end());
                return res;
            }
            
            template<class T, class F>
            std::set<std::pair<std::string, std::string>> parse_table_name(const column_pointer<T, F> &) const {
                std::set<std::pair<std::string, std::string>> res;
                res.insert({this->impl.template find_table_name<T>(), ""});
                return res;
            }
            
            template<class T, class C>
            std::set<std::pair<std::string, std::string>> parse_table_name(const alias_column_t<T, C> &a) const {
                return this->parse_table_name(a.column, alias_extractor<T>::get());
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::count_asterisk_t<T> &) const {
                auto tableName = this->impl.template find_table_name<T>();
                if(!tableName.empty()){
                    return {std::make_pair(std::move(tableName), "")};
                }else{
                    return {};
                }
            }
            
            std::set<std::pair<std::string, std::string>> parse_table_name(const aggregate_functions::count_asterisk_without_type &) const {
                return {};
            }
            
            template<class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const asterisk_t<T> &) const {
                auto tableName = this->impl.template find_table_name<T>();
                return {std::make_pair(std::move(tableName), "")};
            }
            
            template<class T, class E>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::cast_t<T, E> &c) const {
                return this->parse_table_name(c.expression);
            }
            
            template<class R, class T, class E, class ...Args>
            std::set<std::pair<std::string, std::string>> parse_table_name(const simple_case_t<R, T, E, Args...> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                c.case_expression.apply([this, &res](auto &c){
                    auto caseExpressionSet = this->parse_table_name(c);
                    res.insert(caseExpressionSet.begin(), caseExpressionSet.end());
                });
                iterate_tuple(c.args, [this, &res](auto &pair){
                    auto leftSet = this->parse_table_name(pair.first);
                    res.insert(leftSet.begin(), leftSet.end());
                    auto rightSet = this->parse_table_name(pair.second);
                    res.insert(rightSet.begin(), rightSet.end());
                });
                c.else_expression.apply([this, &res](auto &el){
                    auto tableNames = this->parse_table_name(el);
                    res.insert(tableNames.begin(), tableNames.end());
                });
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::and_condition_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::or_condition_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::is_equal_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::is_not_equal_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::greater_than_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::greater_or_equal_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::lesser_than_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class L, class R>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::lesser_or_equal_t<L, R> &c) const {
                std::set<std::pair<std::string, std::string>> res;
                auto leftTableNames = this->parse_table_name(c.l);
                res.insert(leftTableNames.begin(), leftTableNames.end());
                auto rightTableNames = this->parse_table_name(c.r);
                res.insert(rightTableNames.begin(), rightTableNames.end());
                return res;
            }
            
            template<class A, class T, class E>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::like_t<A, T, E> &l) const {
                std::set<std::pair<std::string, std::string>> res;
                auto argTableNames = this->parse_table_name(l.arg);
                res.insert(argTableNames.begin(), argTableNames.end());
                auto patternTableNames = this->parse_table_name(l.pattern);
                res.insert(patternTableNames.begin(), patternTableNames.end());
                l.arg3.apply([&res, this](auto &value){
                    auto escapeTableNames = this->parse_table_name(value);
                    res.insert(escapeTableNames.begin(), escapeTableNames.end());
                });
                return res;
            }
            
            template<class A, class T>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::glob_t<A, T> &l) const {
                std::set<std::pair<std::string, std::string>> res;
                auto argTableNames = this->parse_table_name(l.arg);
                res.insert(argTableNames.begin(), argTableNames.end());
                auto patternTableNames = this->parse_table_name(l.pattern);
                res.insert(patternTableNames.begin(), patternTableNames.end());
                return res;
            }
            
            template<class C>
            std::set<std::pair<std::string, std::string>> parse_table_name(const conditions::negated_condition_t<C> &c) const {
                return this->parse_table_name(c.c);
            }
            
            template<class T, class E>
            std::set<std::pair<std::string, std::string>> parse_table_name(const as_t<T, E> &a) const {
                return this->parse_table_name(a.expression);
            }
            
            template<class ...Args>
            std::set<std::pair<std::string, std::string>> parse_table_name(const internal::columns_t<Args...> &cols) const {
                std::set<std::pair<std::string, std::string>> res;
                iterate_tuple(cols.columns, [&res, this](auto &m){
                    auto tableName = this->parse_table_name(m);
                    res.insert(tableName.begin(), tableName.end());
                });
                return res;
            }
            
            template<class F, class O, class ...Args>
            std::string group_concat_internal(F O::*m, std::unique_ptr<std::string> y, Args&& ...args) {
                this->assert_mapped_type<O>();
                std::vector<std::string> rows;
                if(y){
                    rows = this->select(sqlite_orm::group_concat(m, move(*y)), std::forward<Args>(args)...);
                }else{
                    rows = this->select(sqlite_orm::group_concat(m), std::forward<Args>(args)...);
                }
                if(!rows.empty()){
                    return move(rows.front());
                }else{
                    return {};
                }
            }
            
        public:
            
            /**
             *  Select * with no conditions routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  @return All objects of type O stored in database at the moment.
             */
            template<class O, class C = std::vector<O>, class ...Args>
            C get_all(Args&& ...args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all<O>(std::forward<Args>(args)...));
                return this->execute(statement);
            }
            
            /**
             *  Select * by id routine.
             *  throws std::system_error(orm_error_code::not_found, orm_error_category) if object not found with given id.
             *  throws std::system_error with orm_error_category in case of db error.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  @return Object of type O where id is equal parameter passed or throws `std::system_error(orm_error_code::not_found, orm_error_category)`
             *  if there is no object with such id.
             */
            template<class O, class ...Ids>
            O get(Ids ...ids) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get<O>(std::forward<Ids>(ids)...));
                return this->execute(statement);
            }
            
            /**
             *  The same as `get` function but doesn't throw an exception if noting found but returns std::unique_ptr with null value.
             *  throws std::system_error in case of db error.
             */
            template<class O, class ...Ids>
            std::unique_ptr<O> get_pointer(Ids ...ids) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_pointer<O>(std::forward<Ids>(ids)...));
                return this->execute(statement);
            }

            /**
             * A previous version of get_pointer() that returns a shared_ptr
             * instead of a unique_ptr. New code should prefer get_pointer()
             * unless the data needs to be shared.
             *
             * @note
             * Most scenarios don't need shared ownership of data, so we should prefer
             * unique_ptr when possible. It's more efficient, doesn't require atomic
             * ops for a reference count (which can cause major slowdowns on
             * weakly-ordered platforms like ARM), and can be easily promoted to a
             * shared_ptr, exactly like we're doing here.
             * (Conversely, you _can't_ go from shared back to unique.)
             */
            template<class O, class ...Ids>
            std::shared_ptr<O> get_no_throw(Ids ...ids) {
                return std::shared_ptr<O>(get_pointer<O>(std::forward<Ids>(ids)...));
            }

            /**
             *  SELECT COUNT(*) https://www.sqlite.org/lang_aggfunc.html#count
             *  @return Number of O object in table.
             */
            template<class O, class ...Args, class R = typename mapped_type_proxy<O>::type>
            int count(Args&& ...args) {
                this->assert_mapped_type<R>();
                auto rows = this->select(sqlite_orm::count<R>(), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return rows.front();
                }else{
                    return 0;
                }
            }
            
            /**
             *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
             *  @param m member pointer to class mapped to the storage.
             */
            template<class F, class O, class ...Args>
            int count(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::count(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return rows.front();
                }else{
                    return 0;
                }
            }
            
            /**
             *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return average value from db.
             */
            template<class F, class O, class ...Args>
            double avg(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::avg(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return rows.front();
                }else{
                    return 0;
                }
            }
            
            template<class F, class O>
            std::string group_concat(F O::*m) {
                return this->group_concat_internal(m, {});
            }
            
            /**
             *  GROUP_CONCAT(X) query.  https://www.sqlite.org/lang_aggfunc.html#groupconcat
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return group_concat query result.
             */
            template<class F, class O, class ...Args,
            class Tuple = std::tuple<Args...>,
            typename sfinae = typename std::enable_if<std::tuple_size<Tuple>::value >= 1>::type
            >
            std::string group_concat(F O::*m, Args&& ...args) {
                return this->group_concat_internal(m, {}, std::forward<Args>(args)...);
            }
            
            /**
             *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return group_concat query result.
             */
            template<class F, class O, class ...Args>
            std::string group_concat(F O::*m, std::string y, Args&& ...args) {
                return this->group_concat_internal(m, std::make_unique<std::string>(move(y)), std::forward<Args>(args)...);
            }
            
            template<class F, class O, class ...Args>
            std::string group_concat(F O::*m, const char *y, Args&& ...args) {
                std::unique_ptr<std::string> str;
                if(y){
                    str = std::make_unique<std::string>(y);
                }else{
                    str = std::make_unique<std::string>();
                }
                return this->group_concat_internal(m, move(str), std::forward<Args>(args)...);
            }
            
            /**
             *  MAX(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with max value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> max(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::max(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return std::move(rows.front());
                }else{
                    return {};
                }
            }
            
            /**
             *  MIN(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with min value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> min(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::min(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return std::move(rows.front());
                }else{
                    return {};
                }
            }
            
            /**
             *  SUM(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with sum value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> sum(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                std::vector<std::unique_ptr<double>> rows = this->select(sqlite_orm::sum(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    if(rows.front()){
                        return std::make_unique<Ret>(std::move(*rows.front()));
                    }else{
                        return {};
                    }
                }else{
                    return {};
                }
            }
            
            /**
             *  TOTAL(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return total value (the same as SUM but not nullable. More details here https://www.sqlite.org/lang_aggfunc.html)
             */
            template<class F, class O, class ...Args>
            double total(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::total(m), std::forward<Args>(args)...);
                if(!rows.empty()){
                    return std::move(rows.front());
                }else{
                    return {};
                }
            }
            
            /**
             *  Select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             *  For a single column use `auto rows = storage.select(&User::id, where(...));
             *  For multicolumns use `auto rows = storage.select(columns(&User::id, &User::name), where(...));
             */
            template<
            class T,
            class ...Args,
            class R = typename column_result_t<self, T>::type>
            std::vector<R> select(T m, Args ...args) {
                static_assert(!is_base_of_template<T, compound_operator>::value || std::tuple_size<std::tuple<Args...>>::value == 0,
                              "Cannot use args with a compound operator");
                auto statement = this->prepare(sqlite_orm::select(std::move(m), std::forward<Args>(args)...));
                return this->execute(statement);
            }
            
            /**
             *  Returns a string representation of object of a class mapped to the storage.
             *  Type of string has json-like style.
             */
            template<class O>
            std::string dump(const O &o) {
                this->assert_mapped_type<O>();
                return this->impl.dump(o);
            }
            
            /**
             *  This is REPLACE (INSERT OR REPLACE) function.
             *  Also if you need to insert value with knows id you should
             *  also you this function instead of insert cause inserts ignores
             *  id and creates own one.
             */
            template<class O>
            void replace(const O &o) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::replace(o));
                this->execute(statement);
            }
            
            template<class It>
            void replace_range(It from, It to) {
                using O = typename std::iterator_traits<It>::value_type;
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }
                
                auto statement = this->prepare(sqlite_orm::replace_range(from, to));
                this->execute(statement);
            }
            
            template<class O, class ...Cols>
            int insert(const O &o, columns_t<Cols...> cols) {
                constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                static_assert(colsCount > 0, "Use insert or replace with 1 argument instead");
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(o, std::move(cols)));
                return int(this->execute(statement));
            }
            
            /**
             *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
             *  object doesn't matter.
             *  @return id of just created object.
             */
            template<class O>
            int insert(const O &o) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(o));
                return int(this->execute(statement));
            }
            
            template<class It>
            void insert_range(It from, It to) {
                using O = typename std::iterator_traits<It>::value_type;
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }
                
                auto statement = this->prepare(sqlite_orm::insert_range(from, to));
                this->execute(statement);
            }
            
        protected:
            
            template<class ...Tss, class ...Cols>
            sync_schema_result sync_table(storage_impl<internal::index_t<Cols...>, Tss...> *impl, sqlite3 *db, bool) {
                auto res = sync_schema_result::already_in_sync;
                std::stringstream ss;
                ss << "CREATE ";
                if(impl->table.unique){
                    ss << "UNIQUE ";
                }
                using columns_type = typename decltype(impl->table)::columns_type;
                using head_t = typename std::tuple_element<0, columns_type>::type;
                using indexed_type = typename internal::table_type<head_t>::type;
                ss << "INDEX IF NOT EXISTS '" << impl->table.name << "' ON '" << this->impl.template find_table_name<indexed_type>() << "' ( ";
                std::vector<std::string> columnNames;
                iterate_tuple(impl->table.columns, [&columnNames, this](auto &v){
                    columnNames.push_back(this->impl.column_name(v));
                });
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "'" << columnNames[i] << "'";
                    if(i < columnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << ") ";
                auto query = ss.str();
                auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
                return res;
            }
            
            template<class ...Tss, class ...Cs>
            sync_schema_result sync_table(storage_impl<table_t<Cs...>, Tss...> *impl, sqlite3 *db, bool preserve) {
                auto res = sync_schema_result::already_in_sync;
                
                auto schema_stat = impl->schema_status(db, preserve);
                if(schema_stat != decltype(schema_stat)::already_in_sync) {
                    if(schema_stat == decltype(schema_stat)::new_table_created) {
                        this->create_table(db, impl->table.name, impl);
                        res = decltype(res)::new_table_created;
                    }else{
                        if(schema_stat == sync_schema_result::old_columns_removed
                           || schema_stat == sync_schema_result::new_columns_added
                           || schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed)
                        {
                            
                            //  get table info provided in `make_table` call..
                            auto storageTableInfo = impl->table.get_table_info();
                            
                            //  now get current table info from db using `PRAGMA table_info` query..
                            auto dbTableInfo = impl->get_table_info(impl->table.name, db);
                            
                            //  this vector will contain pointers to columns that gotta be added..
                            std::vector<table_info*> columnsToAdd;
                            
                            impl->get_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);
                            
                            if(schema_stat == sync_schema_result::old_columns_removed) {
                                
                                //  extra table columns than storage columns
                                this->backup_table(db, impl);
                                res = decltype(res)::old_columns_removed;
                            }
                            
                            if(schema_stat == sync_schema_result::new_columns_added) {
                                for(auto columnPointer : columnsToAdd) {
                                    impl->add_column(*columnPointer, db);
                                }
                                res = decltype(res)::new_columns_added;
                            }
                            
                            if(schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {
                                
                                //remove extra columns
                                this->backup_table(db, impl);
                                for(auto columnPointer : columnsToAdd) {
                                    impl->add_column(*columnPointer, db);
                                }
                                res = decltype(res)::new_columns_added_and_old_columns_removed;
                            }
                        } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                            this->drop_table_internal(impl->table.name, db);
                            this->create_table(db, impl->table.name, impl);
                            res = decltype(res)::dropped_and_recreated;
                        }
                    }
                }
                return res;
            }
            
        public:
            
            /**
             *  This is a cute function used to replace migration up/down functionality.
             *  It performs check storage schema with actual db schema and:
             *  * if there are excess tables exist in db they are ignored (not dropped)
             *  * every table from storage is compared with it's db analog and
             *      * if table doesn't exist it is being created
             *      * if table exists its colums are being compared with table_info from db and
             *          * if there are columns in db that do not exist in storage (excess) table will be dropped and recreated
             *          * if there are columns in storage that do not exist in db they will be added using `ALTER TABLE ... ADD COLUMN ...' command
             *          * if there is any column existing in both db and storage but differs by any of properties/constraints (type, pk, notnull, dflt_value) table will be dropped and recreated
             *  Be aware that `sync_schema` doesn't guarantee that data will not be dropped. It guarantees only that it will make db schema the same
             *  as you specified in `make_storage` function call. A good point is that if you have no db file at all it will be created and
             *  all tables also will be created with exact tables and columns you specified in `make_storage`, `make_table` and `make_column` call.
             *  The best practice is to call this function right after storage creation.
             *  @param preserve affects on function behaviour in case it is needed to remove a column. If it is `false` so table will be dropped
             *  if there is column to remove, if `true` -  table is being copied into another table, dropped and copied table is renamed with source table name.
             *  Warning: sync_schema doesn't check foreign keys cause it is unable to do so in sqlite3. If you know how to get foreign key info
             *  please submit an issue https://github.com/fnc12/sqlite_orm/issues
             *  @return std::map with std::string key equal table name and `sync_schema_result` as value. `sync_schema_result` is a enum value that stores
             *  table state after syncing a schema. `sync_schema_result` can be printed out on std::ostream with `operator<<`.
             */
            std::map<std::string, sync_schema_result> sync_schema(bool preserve = false) {
                auto con = this->get_connection();
                std::map<std::string, sync_schema_result> result;
                auto db = con.get();
                this->impl.for_each([&result, db, preserve, this](auto impl){
                    auto res = this->sync_table(impl, db, preserve);
                    result.insert({impl->table.name, res});
                });
                return result;
            }
            
            /**
             *  This function returns the same map that `sync_schema` returns but it
             *  doesn't perform `sync_schema` actually - just simulates it in case you want to know
             *  what will happen if you sync your schema.
             */
            std::map<std::string, sync_schema_result> sync_schema_simulate(bool preserve = false) {
                auto con = this->get_connection();
                std::map<std::string, sync_schema_result> result;
                auto db = con.get();
                this->impl.for_each([&result, db, preserve](auto impl){
                    result.insert({impl->table.name, impl->schema_status(db, preserve)});
                });
                return result;
            }
            
            /**
             *  Checks whether table exists in db. Doesn't check storage itself - works only with actual database.
             *  Note: table can be not mapped to a storage
             *  @return true if table with a given name exists in db, false otherwise.
             */
            bool table_exists(const std::string &tableName) {
                auto con = this->get_connection();
                return this->impl.table_exists(tableName, con.get());
            }
            
            template<class T, class ...Args>
            prepared_statement_t<select_t<T, Args...>> prepare(select_t<T, Args...> sel) {
                sel.highest_level = true;
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(sel, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(sel), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Args>
            prepared_statement_t<get_all_t<T, Args...>> prepare(get_all_t<T, Args...> get) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(get, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(get), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class ...Args, class ...Wargs>
            prepared_statement_t<update_all_t<set_t<Args...>, Wargs...>> prepare(update_all_t<set_t<Args...>, Wargs...> upd) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(upd, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(upd), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Args>
            prepared_statement_t<remove_all_t<T, Args...>> prepare(remove_all_t<T, Args...> rem) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(rem, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(rem), stmt, std::move(con)};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Ids>
            prepared_statement_t<get_t<T, Ids...>> prepare(get_t<T, Ids...> g) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(g, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(g), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Ids>
            prepared_statement_t<get_pointer_t<T, Ids...>> prepare(get_pointer_t<T, Ids...> g) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(g, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(g), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            prepared_statement_t<update_t<T, by_ref>> prepare(update_t<T, by_ref> upd) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(upd, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(upd), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Ids>
            prepared_statement_t<remove_t<T, Ids...>> prepare(remove_t<T, Ids...> rem) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(rem, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(rem), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            prepared_statement_t<insert_t<T, by_ref>> prepare(insert_t<T, by_ref> ins) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(ins, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(ins), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            prepared_statement_t<replace_t<T, by_ref>> prepare(replace_t<T, by_ref> rep) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(rep, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(rep), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class It>
            prepared_statement_t<insert_range_t<It>> prepare(insert_range_t<It> ins) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(ins, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(ins), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class It>
            prepared_statement_t<replace_range_t<It>> prepare(replace_range_t<It> rep) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(rep, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(rep), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref, class ...Cols>
            prepared_statement_t<insert_explicit<T, by_ref, Cols...>> prepare(insert_explicit<T, by_ref, Cols...> ins) {
                auto con = this->get_connection();
                sqlite3_stmt *stmt;
                auto db = con.get();
                auto query = this->string_from_expression(ins, false);
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return {std::move(ins), stmt, con};
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref, class ...Cols>
            int64 execute(const prepared_statement_t<insert_explicit<T, by_ref, Cols...>> &statement) {
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto &impl = this->get_impl<T>();
                auto &o = statement.t.obj;
                sqlite3_reset(stmt);
                iterate_tuple(statement.t.columns.columns, [&o, &index, &stmt, &impl, db] (auto &m) {
                    using column_type = typename std::decay<decltype(m)>::type;
                    using field_type = typename column_result_t<self, column_type>::type;
                    const field_type *value = impl.table.template get_object_field_pointer<field_type>(o, m);
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, *value)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    return sqlite3_last_insert_rowid(db);
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class It>
            void execute(const prepared_statement_t<replace_range_t<It>> &statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_type::object_type;
                auto &impl = this->get_impl<object_type>();
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                sqlite3_reset(stmt);
                for(auto it = statement.t.range.first; it != statement.t.range.second; ++it) {
                    auto &o = *it;
                    impl.table.for_each_column([&o, &index, &stmt, db] (auto &c) {
                        using column_type = typename std::decay<decltype(c)>::type;
                        using field_type = typename column_type::field_type;
                        if(c.member_pointer){
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer)){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }else{
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }
                    });
                }
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class It>
            void execute(const prepared_statement_t<insert_range_t<It>> &statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_type::object_type;
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto &impl = this->get_impl<object_type>();
                sqlite3_reset(stmt);
                for(auto it = statement.t.range.first; it != statement.t.range.second; ++it) {
                    auto &o = *it;
                    impl.table.for_each_column([&o, &index, &stmt, db] (auto &c) {
                        if(!c.template has<constraints::primary_key_t<>>()){
                            using column_type = typename std::decay<decltype(c)>::type;
                            using field_type = typename column_type::field_type;
                            if(c.member_pointer){
                                if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer)){
                                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                                }
                            }else{
                                using getter_type = typename column_type::getter_type;
                                field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                                if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                                }
                            }
                        }
                    });
                }
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            void execute(const prepared_statement_t<replace_t<T, by_ref>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                auto &o = statement.t.obj;
                auto &impl = this->get_impl<T>();
                sqlite3_reset(stmt);
                impl.table.for_each_column([&o, &index, &stmt, db] (auto &c) {
                    using column_type = typename std::decay<decltype(c)>::type;
                    using field_type = typename column_type::field_type;
                    if(c.member_pointer){
                        if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer)){
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    }else{
                        using getter_type = typename column_type::getter_type;
                        field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                        if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            int64 execute(const prepared_statement_t<insert_t<T, by_ref>> &statement) {
                int64 res = 0;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                auto &impl = this->get_impl<T>();
                auto &o = statement.t.obj;
                auto compositeKeyColumnNames = impl.table.composite_key_columns_names();
                sqlite3_reset(stmt);
                impl.table.for_each_column([&o, &index, &stmt, &impl, &compositeKeyColumnNames, db] (auto &c) {
                    if(impl.table._without_rowid || !c.template has<constraints::primary_key_t<>>()){
                        auto it = std::find(compositeKeyColumnNames.begin(),
                                            compositeKeyColumnNames.end(),
                                            c.name);
                        if(it == compositeKeyColumnNames.end()){
                            using column_type = typename std::decay<decltype(c)>::type;
                            using field_type = typename column_type::field_type;
                            if(c.member_pointer){
                                if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer)){
                                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                                }
                            }else{
                                using getter_type = typename column_type::getter_type;
                                field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                                if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                                }
                            }
                        }
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    res = sqlite3_last_insert_rowid(db);
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
                return res;
            }
            
            template<class T, class ...Ids>
            void execute(const prepared_statement_t<remove_t<T, Ids...>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_tuple(statement.t.ids, [stmt, &index, db](auto &v){
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, bool by_ref>
            void execute(const prepared_statement_t<update_t<T, by_ref>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto &impl = this->get_impl<T>();
                auto stmt = statement.stmt;
                auto index = 1;
                auto &o = statement.t.obj;
                sqlite3_reset(stmt);
                impl.table.for_each_column([&o, stmt, &index, db] (auto &c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        using column_type = typename std::decay<decltype(c)>::type;
                        using field_type = typename column_type::field_type;
                        if(c.member_pointer){
                            auto bind_res = statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer);
                            if(SQLITE_OK != bind_res){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }else{
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }
                    }
                });
                impl.table.for_each_column([&o, stmt, &index, db] (auto &c) {
                    if(c.template has<constraints::primary_key_t<>>()) {
                        using column_type = typename std::decay<decltype(c)>::type;
                        using field_type = typename column_type::field_type;
                        if(c.member_pointer){
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, o.*c.member_pointer)){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }else{
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((o).*(c.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)){
                                throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                            }
                        }
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Ids>
            std::unique_ptr<T> execute(const prepared_statement_t<get_pointer_t<T, Ids...>> &statement) {
                auto &impl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_tuple(statement.t.ids, [stmt, &index, db](auto &v){
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes){
                    case SQLITE_ROW:{
                        T res;
                        index = 0;
                        impl.table.for_each_column([&index, &res, stmt] (auto c) {
                            using field_type = typename decltype(c)::field_type;
                            auto value = row_extractor<field_type>().extract(stmt, index++);
                            if(c.member_pointer){
                                res.*c.member_pointer = std::move(value);
                            }else{
                                ((res).*(c.setter))(std::move(value));
                            }
                        });
                        return std::make_unique<T>(std::move(res));
                    }break;
                    case SQLITE_DONE:{
                        return {};
                    }break;
                    default:{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }
            }
            
            template<class T, class ...Ids>
            T execute(const prepared_statement_t<get_t<T, Ids...>> &statement) {
                auto &impl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_tuple(statement.t.ids, [stmt, &index, db](auto &v){
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes){
                    case SQLITE_ROW:{
                        T res;
                        index = 0;
                        impl.table.for_each_column([&index, &res, stmt] (auto &c) {
                            using column_type = typename std::decay<decltype(c)>::type;
                            using field_type = typename column_type::field_type;
                            auto value = row_extractor<field_type>().extract(stmt, index++);
                            if(c.member_pointer){
                                res.*c.member_pointer = std::move(value);
                            }else{
                                ((res).*(c.setter))(std::move(value));
                            }
                        });
                        return res;
                    }break;
                    case SQLITE_DONE:{
                        throw std::system_error(std::make_error_code(sqlite_orm::orm_error_code::not_found));
                    }break;
                    default:{
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                }
            }
            
            template<class T, class ...Args>
            void execute(const prepared_statement_t<remove_all_t<T, Args...>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.t.conditions, [stmt, &index, db](auto &node){
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class ...Args, class ...Wargs>
            void execute(const prepared_statement_t<update_all_t<set_t<Args...>, Wargs...>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                statement.t.set.for_each([&index, stmt, db](auto &setArg){
                    iterate_ast(setArg, [&index, stmt, db](auto &node){
                        using node_type = typename std::decay<decltype(node)>::type;
                        conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                        if(SQLITE_OK != binder(node)){
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    });
                });
                iterate_ast(statement.t.conditions, [stmt, &index, db](auto &node){
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    //  done..
                }else{
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                }
            }
            
            template<class T, class ...Args, class R = typename column_result_t<self, T>::type>
            std::vector<R> execute(const prepared_statement_t<select_t<T, Args...>> &statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.t, [stmt, &index, db](auto &node){
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                std::vector<R> res;
                int stepRes;
                do{
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            res.push_back(row_extractor<R>().extract(stmt, 0));
                        }break;
                        case SQLITE_DONE: break;
                        default:{
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    }
                }while(stepRes != SQLITE_DONE);
                return res;
            }
            
            template<class T, class ...Args>
            std::vector<T> execute(const prepared_statement_t<get_all_t<T, Args...>> &statement) {
                auto &impl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.t, [stmt, &index, db](auto &node){
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)){
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                    }
                });
                std::vector<T> res;
                int stepRes;
                do{
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes){
                        case SQLITE_ROW:{
                            T obj;
                            auto index = 0;
                            impl.table.for_each_column([&index, &obj, stmt] (auto &c) {
                                using field_type = typename std::decay<decltype(c)>::type::field_type;
                                auto value = row_extractor<field_type>().extract(stmt, index++);
                                if(c.member_pointer){
                                    obj.*c.member_pointer = std::move(value);
                                }else{
                                    ((obj).*(c.setter))(std::move(value));
                                }
                            });
                            res.push_back(std::move(obj));
                        }break;
                        case SQLITE_DONE: break;
                        default:{
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()), sqlite3_errmsg(db));
                        }
                    }
                }while(stepRes != SQLITE_DONE);
                return res;
            }
        };
        
        template<class T>
        struct is_storage : std::false_type {};
        
        template<class ...Ts>
        struct is_storage<storage_t<Ts...>> : std::true_type {};
    }
    
    template<class ...Ts>
    internal::storage_t<Ts...> make_storage(const std::string &filename, Ts ...tables) {
        return {filename, internal::storage_impl<Ts...>(tables...)};
    }
    
    /**
     *  sqlite3_threadsafe() interface.
     */
    inline int threadsafe() {
        return sqlite3_threadsafe();
    }
}
