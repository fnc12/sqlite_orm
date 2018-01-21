//
//  storage.h
//  CPPTest
//
//  Created by John Zakharov on 21.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef storage_h
#define storage_h

#include "internal.h"
#include "aggregate_functions.h"

#include <sqlite3.h>

namespace sqlite_orm {
    
    typedef sqlite_int64 int64;
    typedef sqlite_uint64 uint64;
    
    namespace internal {
        
        /**
         *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage` function.
         */
        template<class ...Ts>
        struct storage_t {
            using storage_type = storage_t<Ts...>;
            using impl_type = storage_impl<Ts...>;
            
            template<class T, class ...Args>
            struct view_t {
                using mapped_type = T;
                
                storage_t &storage;
                std::shared_ptr<internal::database_connection> connection;
                
                const std::string query;
                
                view_t(storage_t &stor, decltype(connection) conn, Args&& ...args):
                storage(stor),
                connection(conn),
                query([&]{
                    std::string q;
                    stor.template generate_select_asterisk<T>(&q, args...);
                    return q;
                }()){}
                
                struct iterator_t {
                protected:
                    std::shared_ptr<sqlite3_stmt *> stmt;
                    view_t<T, Args...> &view;
                    std::shared_ptr<T> temp;
                    
                    void extract_value(decltype(temp) &temp) {
                        temp = std::make_shared<T>();
                        auto &storage = this->view.storage;
                        auto &impl = storage.template get_impl<T>();
                        auto index = 0;
                        impl.table.for_each_column([&index, &temp, this] (auto c) {
                            auto value = row_extractor<typename decltype(c)::field_type>().extract(*this->stmt, index++);
                            if(c.member_pointer){
                                auto member_pointer = c.member_pointer;
                                (*temp).*member_pointer = value;
                            }else{
                                ((*temp).*(c.setter))(std::move(value));
                            }
                        });
                    }
                    
                public:
                    iterator_t(sqlite3_stmt * stmt_, view_t<T, Args...> &view_):stmt(std::make_shared<sqlite3_stmt *>(stmt_)),view(view_){
                        this->operator++();
                    }
                    
                    ~iterator_t() {
                        if(this->stmt){
                            statement_finalizer f{*this->stmt};
                        }
                    }
                    
                    T& operator*() {
                        if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                        if(!this->temp){
                            this->extract_value(this->temp);
                        }
                        return *this->temp;
                    }
                    
                    T* operator->() {
                        if(!this->stmt) throw std::runtime_error("trying to dereference null iterator");
                        if(!this->temp){
                            this->extract_value(this->temp);
                        }
                        return &*this->temp;
                    }
                    
                    void operator++() {
                        if(this->stmt && *this->stmt){
                            auto ret = sqlite3_step(*this->stmt);
                            switch(ret){
                                case SQLITE_ROW:
                                    this->temp = nullptr;
                                    break;
                                case SQLITE_DONE:{
                                    statement_finalizer f{*this->stmt};
                                    *this->stmt = nullptr;
                                }break;
                                default:{
                                    auto db = this->view.connection->get_db();
                                    auto msg = sqlite3_errmsg(db);
                                    throw std::runtime_error(msg);
                                }
                            }
                        }
                    }
                    
                    void operator++(int) {
                        this->operator++();
                    }
                    
                    bool operator==(const iterator_t &other) const {
                        if(this->stmt && other.stmt){
                            return *this->stmt == *other.stmt;
                        }else{
                            if(!this->stmt && !other.stmt){
                                return true;
                            }else{
                                return false;
                            }
                        }
                    }
                    
                    bool operator!=(const iterator_t &other) const {
                        return !(*this == other);
                    }
                };
                
                size_t size() {
                    return this->storage.template count<T>();
                }
                
                bool empty() {
                    return !this->size();
                }
                
                iterator_t end() {
                    return {nullptr, *this};
                }
                
                iterator_t begin() {
                    sqlite3_stmt *stmt = nullptr;
                    auto db = this->connection->get_db();
                    auto ret = sqlite3_prepare_v2(db, this->query.c_str(), -1, &stmt, nullptr);
                    if(ret == SQLITE_OK){
                        return {stmt, *this};
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }
            };
            
            struct transaction_guard_t {
                typedef storage_t<Ts...> storage_type;
                
                /**
                 *  This is a public lever to tell a guard what it must do in its destructor
                 *  if `gotta_fire` is true
                 */
                bool commit_on_destroy = false;
                
                transaction_guard_t(storage_type &s):storage(s){}
                
                ~transaction_guard_t() {
                    if(this->gotta_fire){
                        if(!this->commit_on_destroy){
                            this->storage.rollback();
                        }else{
                            this->storage.commit();
                        }
                    }
                }
                
                /**
                 *  Call `COMMIT` explicitly. After this call
                 *  guard will not call `COMMIT` or `ROLLBACK`
                 *  in its destructor.
                 */
                void commit() {
                    this->storage.commit();
                    this->gotta_fire = false;
                }
                
                /**
                 *  Call `ROLLBACK` explicitly. After this call
                 *  guard will not call `COMMIT` or `ROLLBACK`
                 *  in its destructor.
                 */
                void rollback() {
                    this->storage.rollback();
                    this->gotta_fire = false;
                }
                
            protected:
                storage_type &storage;
                bool gotta_fire = true;
            };
            
            std::function<void(sqlite3*)> on_open;
            
            transaction_guard_t transaction_guard() {
                this->begin_transaction();
                return {*this};
            }
            
            struct pragma_t {
                
                pragma_t(storage_type &storage_):storage(storage_){}
                
                int synchronous() {
                    return this->get_pragma<int>("synchronous");
                }
                
                void synchronous(int value) {
                    this->_synchronous = -1;
                    this->set_pragma("synchronous", value);
                    this->_synchronous = value;
                }
                
                int user_version() {
                    return this->get_pragma<int>("user_version");
                }
                
                void user_version(int value) {
                    this->set_pragma("user_version", value);
                }
                
                friend class storage_t<Ts...>;
                
            protected:
                storage_type &storage;
                int _synchronous = -1;
                
                template<class T>
                T get_pragma(const std::string &name) {
                    auto connection = this->storage.get_or_create_connection();
                    std::string query = "PRAGMA " + name;
                    int res = -1;
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **) -> int {
                                               auto &res = *(T*)data;
                                               if(argc){
                                                   res = row_extractor<T>().extract(argv[0]);
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                    return res;
                }
                
                template<class T>
                void set_pragma(const std::string &name, const T &value) {
                    auto connection = this->storage.get_or_create_connection();
                    std::stringstream ss;
                    ss << "PRAGMA " << name << " = " << this->storage.string_from_expression(value);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(), query.c_str(), nullptr, nullptr, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }
            };
            
            /**
             *  @param filename_ database filename.
             */
            storage_t(const std::string &filename_, impl_type impl_):
            filename(filename_),
            impl(impl_),
            inMemory(filename_.empty() || filename_ == ":memory:"),
            pragma(*this){
                if(inMemory){
                    this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                    this->on_open_internal(this->currentTransaction->get_db());
                }
            }
            
        protected:
            
            /**
             *  Check whether connection exists and returns it if yes or creates a new one
             *  and returns it.
             */
            std::shared_ptr<internal::database_connection> get_or_create_connection() {
                decltype(this->currentTransaction) connection;
                if(!this->currentTransaction){
                    connection = std::make_shared<internal::database_connection>(this->filename);
                    this->on_open_internal(connection->get_db());
                }else{
                    connection = this->currentTransaction;
                }
                return connection;
            }
            
            template<class O, class T, class ...Op>
            std::string serialize_column_schema(internal::column_t<O, T, Op...> c) {
                std::stringstream ss;
                ss << "'" << c.name << "' ";
                typedef typename decltype(c)::field_type field_type;
                typedef typename decltype(c)::constraints_type constraints_type;
                ss << type_printer<field_type>().print() << " ";
                tuple_helper::iterator<std::tuple_size<constraints_type>::value - 1, Op...>()(c.constraints, [&](auto &v){
                    ss << static_cast<std::string>(v) << ' ';
                });
                if(c.not_null()){
                    ss << "NOT NULL ";
                }
                return ss.str();
            }
            
            template<class ...Cs>
            std::string serialize_column_schema(constraints::primary_key_t<Cs...> fk) {
                std::stringstream ss;
                ss << static_cast<std::string>(fk) << " (";
                std::vector<std::string> columnNames;
                columnNames.reserve(std::tuple_size<decltype(fk.columns)>::value);
                tuple_helper::iterator<std::tuple_size<decltype(fk.columns)>::value - 1, Cs...>()(fk.columns, [&](auto &c){
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
            
            template<class C, class R>
            std::string serialize_column_schema(constraints::foreign_key_t<C, R> fk) {
                std::stringstream ss;
                typedef typename internal::table_type<decltype(fk.r)>::type ref_type;
                auto refTableName = this->impl.template find_table_name<ref_type>();
                auto refColumnName = this->impl.column_name(fk.r);
                ss << "FOREIGN KEY(" << this->impl.column_name(fk.m) << ") REFERENCES ";
                ss << refTableName << "(" << refColumnName << ") ";
                return ss.str();
            }
#endif
            
            template<class I>
            void create_table(sqlite3 *db, const std::string &tableName, I *impl) {
                std::stringstream ss;
                ss << "CREATE TABLE '" << tableName << "' ( ";
                auto columnsCount = impl->table.columns_count();
                auto index = 0;
                impl->table.for_each_column_with_constraints([columnsCount, &index, &ss, this] (auto c) {
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
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
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
            void assert_mapped_type() {
                typedef std::tuple<typename Ts::object_type...> mapped_types_tuples;
                static_assert(tuple_helper::has_type<O, mapped_types_tuples>::value, "type is not mapped to a storage");
            }
            
            template<class O>
            auto& get_impl() {
                return this->impl.template get_impl<O>();
            }
            
            std::string escape(std::string text) {
                for(size_t i = 0; i < text.length(); ) {
                    if(text[i] == '\''){
                        text.insert(text.begin() + i, '\'');
                        i += 2;
                    }
                    else
                        ++i;
                }
                return text;
            }
            
            template<class T>
            std::string string_from_expression(T t, bool /*noTableName*/ = false, bool escape = false) {
                auto isNullable = type_is_nullable<T>::value;
                if(isNullable && !type_is_nullable<T>()(t)){
                    return "NULL";
                }else{
                    auto needQuotes = std::is_base_of<text_printer, type_printer<T>>::value;
                    std::stringstream ss;
                    if(needQuotes){
                        ss << "'";
                    }
                    std::string text = field_printer<T>()(t);
                    if(escape){
                        text = this->escape(text);
                    }
                    ss << text;
                    if(needQuotes){
                        ss << "'";
                    }
                    return ss.str();
                }
            }
            
            std::string string_from_expression(const std::string &t, bool /*noTableName*/ = false, bool escape = false) {
                std::stringstream ss;
                std::string text = t;
                if(escape){
                    text = this->escape(text);
                }
                ss << "'" << text << "'";
                return ss.str();
            }
            
            std::string string_from_expression(const char *t, bool /*noTableName*/ = false, bool escape = false) {
                std::stringstream ss;
                std::string text = t;
                if(escape){
                    text = this->escape(text);
                }
                ss << "'" << text << "'";
                return ss.str();
            }
            
            template<class F, class O>
            std::string string_from_expression(F O::*m, bool noTableName = false, bool /*escape*/ = false) {
                std::stringstream ss;
                if(!noTableName){
                    ss << " '" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << "\"" << this->impl.column_name(m) << "\"";
                return ss.str();
            }
            
            template<class F, class O>
            std::string string_from_expression(const F* (O::*g)() const, bool noTableName = false, bool /*escape*/ = false) {
                std::stringstream ss;
                if(!noTableName){
                    ss << " '" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << "\"" << this->impl.column_name(g) << "\"";
                return ss.str();
            }
            
            template<class F, class O>
            std::string string_from_expression(void (O::*s)(F), bool noTableName = false, bool /*escape*/ = false) {
                std::stringstream ss;
                if(!noTableName){
                    ss << " '" << this->impl.template find_table_name<O>() << "'.";
                }
                ss << "\"" << this->impl.column_name(s) << "\"";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::group_concat_double_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                auto expr2 = this->string_from_expression(f.y);
                ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::group_concat_single_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class L, class R>
            std::string string_from_expression(internal::conc_t<L, R> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto lhs = this->string_from_expression(f.l);
                auto rhs = this->string_from_expression(f.r);
                ss << "(" << lhs << " || " << rhs << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::min_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::max_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::total_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::sum_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            std::string string_from_expression(aggregate_functions::count_asterisk_t &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                ss << static_cast<std::string>(f) << "(*) ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::count_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(aggregate_functions::avg_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(a.t);
                ss << static_cast<std::string>(a) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(internal::distinct_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(internal::all_t<T> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.t);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class X, class Y>
            std::string string_from_expression(core_functions::rtrim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                auto expr2 = this->string_from_expression(f.y);
                ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
                return ss.str();
            }
            
            template<class X>
            std::string string_from_expression(core_functions::rtrim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class X, class Y>
            std::string string_from_expression(core_functions::ltrim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                auto expr2 = this->string_from_expression(f.y);
                ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
                return ss.str();
            }
            
            template<class X>
            std::string string_from_expression(core_functions::ltrim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class X, class Y>
            std::string string_from_expression(core_functions::trim_double_t<X, Y> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                auto expr2 = this->string_from_expression(f.y);
                ss << static_cast<std::string>(f) << "(" << expr << ", " << expr2 << ") ";
                return ss.str();
            }
            
            template<class X>
            std::string string_from_expression(core_functions::trim_single_t<X> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(f.x);
                ss << static_cast<std::string>(f) << "(" << expr << ") ";
                return ss.str();
            }
            
            std::string string_from_expression(core_functions::changes_t &ch, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                ss << static_cast<std::string>(ch) << "() ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(core_functions::length_t<T> &len, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(len.t);
                ss << static_cast<std::string>(len) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T, class ...Args>
            std::string string_from_expression(core_functions::datetime_t<T, Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                ss << static_cast<std::string>(f) << "(" << this->string_from_expression(f.timestring);
                typedef std::tuple<Args...> tuple_t;
                tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                    ss << ", " << this->string_from_expression(v);
                });
                ss << ") ";
                return ss.str();
            }
            
            template<class T, class ...Args>
            std::string string_from_expression(core_functions::date_t<T, Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                ss << static_cast<std::string>(f) << "(" << this->string_from_expression(f.timestring);
                typedef std::tuple<Args...> tuple_t;
                tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                    ss << ", " << this->string_from_expression(v);
                });
                ss << ") ";
                return ss.str();
            }
            
            std::string string_from_expression(core_functions::random_t &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                ss << static_cast<std::string>(f) << "() ";
                return ss.str();
            }
            
#if SQLITE_VERSION_NUMBER >= 3007016
            
            template<class ...Args>
            std::string string_from_expression(core_functions::char_t_<Args...> &f, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                typedef decltype(f.args) tuple_t;
                std::vector<std::string> args;
                args.reserve(std::tuple_size<tuple_t>::value);
                tuple_helper::tuple_for_each(f.args, [&](auto &v){
                    auto expression = this->string_from_expression(v);
                    args.emplace_back(std::move(expression));
                });
                ss << static_cast<std::string>(f) << "(";
                auto lim = int(args.size());
                for(auto i = 0; i < lim; ++i) {
                    ss << args[i];
                    if(i < lim - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << ") ";
                return ss.str();
            }
#endif
            
            template<class T>
            std::string string_from_expression(core_functions::upper_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(a.t);
                ss << static_cast<std::string>(a) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(core_functions::lower_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(a.t);
                ss << static_cast<std::string>(a) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string string_from_expression(core_functions::abs_t<T> &a, bool /*noTableName*/ = false, bool /*escape*/ = false) {
                std::stringstream ss;
                auto expr = this->string_from_expression(a.t);
                ss << static_cast<std::string>(a) << "(" << expr << ") ";
                return ss.str();
            }
            
            template<class T>
            std::string process_where(conditions::is_null_t<T> &c) {
                std::stringstream ss;
                ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
                return ss.str();
            }
            
            template<class T>
            std::string process_where(conditions::is_not_null_t<T> &c) {
                std::stringstream ss;
                ss << this->string_from_expression(c.t) << " " << static_cast<std::string>(c) << " ";
                return ss.str();
            }
            
            template<class C>
            std::string process_where(conditions::negated_condition_t<C> &c) {
                std::stringstream ss;
                ss << " " << static_cast<std::string>(c) << " ";
                auto cString = this->process_where(c.c);
                ss << " (" << cString << " ) ";
                return ss.str();
            }
            
            template<class L, class R>
            std::string process_where(conditions::and_condition_t<L, R> &c) {
                std::stringstream ss;
                ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
                return ss.str();
            }
            
            template<class L, class R>
            std::string process_where(conditions::or_condition_t<L, R> &c) {
                std::stringstream ss;
                ss << " (" << this->process_where(c.l) << ") " << static_cast<std::string>(c) << " (" << this->process_where(c.r) << ") ";
                return ss.str();
            }
            
            /**
             *  Common case. Is used to process binary conditions like is_equal, not_equal
             */
            template<class C>
            std::string process_where(C c) {
                auto leftString = this->string_from_expression(c.l, false, true);
                auto rightString = this->string_from_expression(c.r, false, true);
                std::stringstream ss;
                ss << leftString << " " << static_cast<std::string>(c) << " " << rightString;
                return ss.str();
            }
            
            template<class T>
            std::string process_where(conditions::collate_t<T> &col) {
                auto res = this->process_where(col.expr);
                return res + " " + static_cast<std::string>(col);
            }
            
            template<class L, class E>
            std::string process_where(conditions::in_t<L, E> &inCondition) {
                std::stringstream ss;
                auto leftString = this->string_from_expression(inCondition.l);
                ss << leftString << " " << static_cast<std::string>(inCondition) << " (";
                for(size_t index = 0; index < inCondition.values.size(); ++index) {
                    auto &value = inCondition.values[index];
                    ss << " " << this->string_from_expression(value);
                    if(index < inCondition.values.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << " )";
                return ss.str();
            }
            
            template<class A, class T>
            std::string process_where(conditions::like_t<A, T> &l) {
                std::stringstream ss;
                ss << this->string_from_expression(l.a) << " " << static_cast<std::string>(l) << " " << this->string_from_expression(l.t) << " ";
                return ss.str();
            }
            
            template<class A, class T>
            std::string process_where(conditions::between_t<A, T> &bw) {
                std::stringstream ss;
                auto expr = this->string_from_expression(bw.expr);
                ss << expr << " " << static_cast<std::string>(bw) << " " << this->string_from_expression(bw.b1) << " AND " << this->string_from_expression(bw.b2) << " ";
                return ss.str();
            }
            
            template<class O>
            std::string process_order_by(conditions::order_by_t<O> &orderBy) {
                std::stringstream ss;
                auto columnName = this->string_from_expression(orderBy.o);
                ss << columnName << " ";
                if(orderBy._collate_argument){
                    constraints::collate_t col(*orderBy._collate_argument);
                    ss << static_cast<std::string>(col) << " ";
                }
                switch(orderBy.asc_desc){
                    case 1:
                        ss << "ASC ";
                        break;
                    case -1:
                        ss << "DESC ";
                        break;
                }
                return ss.str();
            }
            
            template<class T>
            void process_join_constraint(std::stringstream &ss, conditions::on_t<T> &t) {
                ss << static_cast<std::string>(t) << " " << this->process_where(t.t) << " ";
            }
            
            template<class F, class O>
            void process_join_constraint(std::stringstream &ss, conditions::using_t<F, O> &u) {
                ss << static_cast<std::string>(u) << " (" << this->string_from_expression(u.column, true) << " ) ";
            }
            
            void process_single_condition(std::stringstream &ss, conditions::limit_t limt) {
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
                ss << " ";
            }
            
            template<class O>
            void process_single_condition(std::stringstream &ss, conditions::cross_join_t<O> c) {
                ss << static_cast<std::string>(c) << " ";
                ss << " '" << this->impl.template find_table_name<O>() << "' ";
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, conditions::inner_join_t<T, O> l) {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, conditions::left_outer_join_t<T, O> l) {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, conditions::left_join_t<T, O> l) {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            template<class T, class O>
            void process_single_condition(std::stringstream &ss, conditions::join_t<T, O> l) {
                ss << static_cast<std::string>(l) << " ";
                ss << " '" << this->impl.template find_table_name<T>() << "' ";
                this->process_join_constraint(ss, l.constraint);
            }
            
            /*template<class T>
             void process_single_condition(std::stringstream &ss, conditions::natural_join_t<T> l) {
             ss << static_cast<std::string>(l) << " ";
             ss << " '" << this->impl.template find_table_name<T>() << "' ";
             //            this->process_join_constraint(ss, l.constraint);
             }*/
            
            template<class C>
            void process_single_condition(std::stringstream &ss, conditions::where_t<C> w) {
                ss << static_cast<std::string>(w) << " ";
                auto whereString = this->process_where(w.c);
                ss << "( " << whereString << ") ";
            }
            
            template<class O>
            void process_single_condition(std::stringstream &ss, conditions::order_by_t<O> orderBy) {
                ss << static_cast<std::string>(orderBy) << " ";
                auto orderByString = this->process_order_by(orderBy);
                ss << orderByString << " ";
            }
            
            template<class ...Args>
            void process_single_condition(std::stringstream &ss, conditions::group_by_t<Args...> groupBy) {
                std::vector<std::string> expressions;
                typedef std::tuple<Args...> typle_t;
                tuple_helper::iterator<std::tuple_size<typle_t>::value - 1, Args...>()(groupBy.args, [&](auto &v){
                    auto expression = this->string_from_expression(v);
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
            
            /**
             *  Recursion end.
             */
            template<class ...Args>
            void process_conditions(std::stringstream &, Args .../*args*/) {
                //..
            }
            
            template<class C, class ...Args>
            void process_conditions(std::stringstream &ss, C c, Args&& ...args) {
                this->process_single_condition(ss, std::move(c));
                this->process_conditions(ss, std::forward<Args>(args)...);
            }
            
            void on_open_internal(sqlite3 *db) {
                
#if SQLITE_VERSION_NUMBER >= 3006019
                if(this->foreign_keys_count()){
                    this->foreign_keys(db, true);
                }
#endif
                if(this->pragma._synchronous != -1) {
                    this->pragma.synchronous(this->pragma._synchronous);
                }
                
                if(this->on_open){
                    this->on_open(db);
                }
                
            }
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            //  returns foreign keys count in storage definition
            int foreign_keys_count() {
                auto res = 0;
                this->impl.for_each([&res](auto impl){
                    res += impl->foreign_keys_count();
                });
                return res;
            }
#endif
            
        public:
            
            template<class T, class ...Args>
            view_t<T, Args...> iterate(Args&& ...args) {
                this->assert_mapped_type<T>();
                
                auto connection = this->get_or_create_connection();
                return {*this, connection, std::forward<Args>(args)...};
            }
            
            template<class O, class ...Args>
            void remove_all(Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::stringstream ss;
                ss << "DELETE FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, std::forward<Args>(args)...);
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            /**
             *  Delete routine.
             *  O is an object's type. Must be specified explicitly.
             *  @param id id of object to be removed.
             */
            template<class O, class I>
            void remove(I id) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::stringstream ss;
                ss << "DELETE FROM '" << impl.table.name << "' ";
                ss << "WHERE ";
                std::vector<std::string> primaryKeyColumnNames;
                impl.table.for_each_column([&] (auto c) {
                    if(c.template has<constraints::primary_key_t<>>()) {
                        primaryKeyColumnNames.emplace_back(c.name);
                    }
                });
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " =  ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " AND ";
                    }else{
                        ss << " ";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    impl.table.template for_each_column_with<constraints::primary_key_t<>>([&] (auto c) {
                        typedef typename decltype(c)::field_type field_type;
                        statement_binder<field_type>().bind(stmt, index++, id);
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
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
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::stringstream ss;
                ss << "UPDATE '" << impl.table.name << "' SET ";
                std::vector<std::string> setColumnNames;
                impl.table.for_each_column([&](auto c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        setColumnNames.emplace_back(c.name);
                    }
                });
                for(size_t i = 0; i < setColumnNames.size(); ++i) {
                    ss << "\"" << setColumnNames[i] << "\"" << " = ?";
                    if(i < setColumnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << "WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                    ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ?";
                    if(i < primaryKeyColumnNames.size() - 1) {
                        ss << " AND ";
                    }else{
                        ss << " ";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    impl.table.for_each_column([&o, stmt, &index] (auto c) {
                        if(!c.template has<constraints::primary_key_t<>>()) {
                            typedef typename decltype(c)::field_type field_type;
                            const field_type *value = nullptr;
                            if(c.member_pointer){
                                value = &(o.*c.member_pointer);
                            }else{
                                value = &((o).*(c.getter))();
                            }
                            statement_binder<field_type>().bind(stmt, index++, *value);
                        }
                    });
                    impl.table.for_each_column([&o, stmt, &index] (auto c) {
                        if(c.template has<constraints::primary_key_t<>>()) {
                            typedef typename decltype(c)::field_type field_type;
                            const field_type *value = nullptr;
                            if(c.member_pointer){
                                value = &(o.*c.member_pointer);
                            }else{
                                value = &((o).*(c.getter))();
                            }
                            statement_binder<field_type>().bind(stmt, index++, *value);
                        }
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            template<class ...Args, class ...Wargs>
            void update_all(internal::set_t<Args...> set, Wargs ...wh) {
                auto connection = this->get_or_create_connection();
                
                std::stringstream ss;
                ss << "UPDATE ";
                std::set<std::string> tableNamesSet;
                set.for_each([this, &tableNamesSet](auto &lhs, auto &/*rhs*/){
                    auto tableName = this->parse_table_name(lhs);
                    tableNamesSet.insert(tableName.begin(), tableName.end());
                });
                if(tableNamesSet.size()){
                    if(tableNamesSet.size() == 1){
                        ss << " '" << *tableNamesSet.begin() << "' ";
                        ss << static_cast<std::string>(set) << " ";
                        std::vector<std::string> setPairs;
                        set.for_each([this, &setPairs](auto &lhs, auto &rhs){
                            std::stringstream sss;
                            sss << this->string_from_expression(lhs, true) << " = " << this->string_from_expression(rhs) << " ";
                            setPairs.push_back(sss.str());
                        });
                        auto setPairsCount = setPairs.size();
                        for(size_t i = 0; i < setPairsCount; ++i) {
                            ss << setPairs[i] << " ";
                            if(i < setPairsCount - 1) {
                                ss << ", ";
                            }
                        }
                        this->process_conditions(ss, wh...);
                        auto query = ss.str();
                        sqlite3_stmt *stmt;
                        if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                            statement_finalizer finalizer{stmt};
                            if (sqlite3_step(stmt) == SQLITE_DONE) {
                                return;
                            }else{
                                auto msg = sqlite3_errmsg(connection->get_db());
                                throw std::runtime_error(msg);
                            }
                        }else {
                            auto msg = sqlite3_errmsg(connection->get_db());
                            throw std::runtime_error(msg);
                        }
                    }else{
                        throw std::runtime_error("too many tables specified - UPDATE can be performed only for a single table");
                    }
                }else{
                    throw std::runtime_error("incorrect SET fields specified");
                }
            }
            
        protected:
            
            /**
             *  O - mapped type
             *  Args - conditions
             *  @param query - result query string
             *  @return impl for O
             */
            template<class O, class ...Args>
            auto& generate_select_asterisk(std::string *query, Args&& ...args) {
                std::stringstream ss;
                ss << "SELECT ";
                auto &impl = this->get_impl<O>();
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss
                    << "'" << impl.table.name << "'."
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
                this->process_conditions(ss, std::forward<Args>(args)...);
                if(query){
                    *query = ss.str();
                }
                return impl;
            }
            
            template<class T>
            std::set<std::string> parse_table_name(T &) {
                return {};
            }
            
            template<class F, class O>
            std::set<std::string> parse_table_name(F O::*) {
                return {this->impl.template find_table_name<O>()};
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::min_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::max_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::sum_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::total_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::group_concat_double_t<T> &f) {
                auto res = this->parse_table_name(f.t);
                auto secondSet = this->parse_table_name(f.y);
                res.insert(secondSet.begin(), secondSet.end());
                return res;
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::group_concat_single_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::count_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(aggregate_functions::avg_t<T> &a) {
                return this->parse_table_name(a.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(core_functions::length_t<T> &len) {
                return this->parse_table_name(len.t);
            }
            
            template<class T, class ...Args>
            std::set<std::string> parse_table_name(core_functions::date_t<T, Args...> &f) {
                auto res = this->parse_table_name(f.timestring);
                typedef decltype(f.modifiers) tuple_t;
                tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                    auto tableNames = this->parse_table_name(v);
                    res.insert(tableNames.begin(), tableNames.end());
                });
                return res;
            }
            
            template<class T, class ...Args>
            std::set<std::string> parse_table_name(core_functions::datetime_t<T, Args...> &f) {
                auto res = this->parse_table_name(f.timestring);
                typedef decltype(f.modifiers) tuple_t;
                tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.modifiers, [&](auto &v){
                    auto tableNames = this->parse_table_name(v);
                    res.insert(tableNames.begin(), tableNames.end());
                });
                return res;
            }
            
            template<class X>
            std::set<std::string> parse_table_name(core_functions::trim_single_t<X> &f) {
                return this->parse_table_name(f.x);
            }
            
            template<class X, class Y>
            std::set<std::string> parse_table_name(core_functions::trim_double_t<X, Y> &f) {
                auto res = this->parse_table_name(f.x);
                auto res2 = this->parse_table_name(f.y);
                res.insert(res2.begin(), res2.end());
                return res;
            }
            
            template<class X>
            std::set<std::string> parse_table_name(core_functions::rtrim_single_t<X> &f) {
                return this->parse_table_name(f.x);
            }
            
            template<class X, class Y>
            std::set<std::string> parse_table_name(core_functions::rtrim_double_t<X, Y> &f) {
                auto res = this->parse_table_name(f.x);
                auto res2 = this->parse_table_name(f.y);
                res.insert(res2.begin(), res2.end());
                return res;
            }
            
            template<class X>
            std::set<std::string> parse_table_name(core_functions::ltrim_single_t<X> &f) {
                return this->parse_table_name(f.x);
            }
            
            template<class X, class Y>
            std::set<std::string> parse_table_name(core_functions::ltrim_double_t<X, Y> &f) {
                auto res = this->parse_table_name(f.x);
                auto res2 = this->parse_table_name(f.y);
                res.insert(res2.begin(), res2.end());
                return res;
            }
            
#if SQLITE_VERSION_NUMBER >= 3007016
            
            template<class ...Args>
            std::set<std::string> parse_table_name(core_functions::char_t_<Args...> &f) {
                std::set<std::string> res;
                typedef decltype(f.args) tuple_t;
                tuple_helper::iterator<std::tuple_size<tuple_t>::value - 1, Args...>()(f.args, [&](auto &v){
                    auto tableNames = this->parse_table_name(v);
                    res.insert(tableNames.begin(), tableNames.end());
                });
                return res;
            }
            
#endif
            
            std::set<std::string> parse_table_name(core_functions::random_t &f) {
                return {};
            }
            
            template<class T>
            std::set<std::string> parse_table_name(core_functions::upper_t<T> &a) {
                return this->parse_table_name(a.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(core_functions::lower_t<T> &a) {
                return this->parse_table_name(a.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(core_functions::abs_t<T> &a) {
                return this->parse_table_name(a.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(internal::distinct_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class T>
            std::set<std::string> parse_table_name(internal::all_t<T> &f) {
                return this->parse_table_name(f.t);
            }
            
            template<class ...Args>
            std::set<std::string> parse_table_names(Args...) {
                return {};
            }
            
            template<class H, class ...Args>
            std::set<std::string> parse_table_names(H h, Args&& ...args) {
                auto res = this->parse_table_names(std::forward<Args>(args)...);
                auto tableName = this->parse_table_name(h);
                res.insert(tableName.begin(),
                           tableName.end());
                return res;
            }
            
            template<class ...Args>
            std::set<std::string> parse_table_names(internal::columns_t<Args...> &cols) {
                std::set<std::string> res;
                cols.for_each([&](auto &m){
                    auto tableName = this->parse_table_name(m);
                    res.insert(tableName.begin(),
                               tableName.end());
                });
                return res;
            }
            
            template<class F, class O, class ...Args>
            std::string group_concat_internal(F O::*m, std::shared_ptr<const std::string> y, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::string res;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::group_concat_single_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName;
                    if(y){
                        ss << ",\"" << *y << "\"";
                    }
                    ss << ") FROM '"<< impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **) -> int {
                                               auto &res = *(std::string*)data;
                                               if(argc){
                                                   res = row_extractor<std::string>().extract(argv[0]);
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
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
                
                auto connection = this->get_or_create_connection();
                C res;
                std::string query;
                auto &impl = this->generate_select_asterisk<O>(&query, std::forward<Args>(args)...);
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    int stepRes;
                    do{
                        stepRes = sqlite3_step(stmt);
                        switch(stepRes){
                            case SQLITE_ROW:{
                                O obj;
                                auto index = 0;
                                impl.table.for_each_column([&index, &obj, stmt] (auto c) {
                                    typedef typename decltype(c)::field_type field_type;
                                    auto value = row_extractor<field_type>().extract(stmt, index++);
                                    if(c.member_pointer){
                                        obj.*c.member_pointer = value;
                                    }else{
                                        ((obj).*(c.setter))(std::move(value));
                                    }
                                });
                                res.push_back(std::move(obj));
                            }break;
                            case SQLITE_DONE: break;
                            default:{
                                auto msg = sqlite3_errmsg(connection->get_db());
                                throw std::runtime_error(msg);
                            }
                        }
                    }while(stepRes != SQLITE_DONE);
                    return res;
                }else{
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            /**
             *  Select * by id routine.
             *  throws sqlite_orm::not_found_exeption if object not found with given id.
             *  throws std::runtime_error in case of db error.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  @return Object of type O where id is equal parameter passed or throws `not_found_exception`
             *  if there is no object with such id.
             */
            template<class O, class ...Ids>
            O get(Ids ...ids) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::shared_ptr<O> res;
                std::stringstream ss;
                ss << "SELECT ";
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << "FROM '" << impl.table.name << "' WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                if(primaryKeyColumnNames.size()){
                    for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                        ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                        if(i < primaryKeyColumnNames.size() - 1) {
                            ss << "AND ";
                        }
                        ss << ' ';
                    }
                    auto query = ss.str();
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                        statement_finalizer finalizer{stmt};
                        auto index = 1;
                        auto idsTuple = std::make_tuple(std::forward<Ids>(ids)...);
                        constexpr const auto idsCount = std::tuple_size<decltype(idsTuple)>::value;
                        tuple_helper::iterator<idsCount - 1, Ids...>()(idsTuple, [stmt, &index](auto &v){
                            typedef typename std::remove_reference<decltype(v)>::type field_type;
                            statement_binder<field_type>().bind(stmt, index++, v);
                        });
                        auto stepRes = sqlite3_step(stmt);
                        switch(stepRes){
                            case SQLITE_ROW:{
                                O res;
                                index = 0;
                                impl.table.for_each_column([&index, &res, stmt] (auto c) {
                                    typedef typename decltype(c)::field_type field_type;
                                    auto value = row_extractor<field_type>().extract(stmt, index++);
                                    if(c.member_pointer){
                                        res.*c.member_pointer = value;
                                    }else{
                                        ((res).*(c.setter))(std::move(value));
                                    }
                                });
                                return res;
                            }break;
                            case SQLITE_DONE:{
                                throw not_found_exception{};
                            }break;
                            default:{
                                auto msg = sqlite3_errmsg(connection->get_db());
                                throw std::runtime_error(msg);
                            }
                        }
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("table " + impl.table.name + " has no primary key column");
                }
            }
            
            /**
             *  The same as `get` function but doesn't throw an exeption if noting found but returns std::shared_ptr with null value.
             *  throws std::runtime_error iin case of db error.
             */
            template<class O, class ...Ids>
            std::shared_ptr<O> get_no_throw(Ids ...ids) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::shared_ptr<O> res;
                std::stringstream ss;
                ss << "SELECT ";
                auto columnNames = impl.table.column_names();
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << "FROM '" << impl.table.name << "' WHERE ";
                auto primaryKeyColumnNames = impl.table.primary_key_column_names();
                if(primaryKeyColumnNames.size() && primaryKeyColumnNames.front().length()){
                    for(size_t i = 0; i < primaryKeyColumnNames.size(); ++i) {
                        ss << "\"" << primaryKeyColumnNames[i] << "\"" << " = ? ";
                        if(i < primaryKeyColumnNames.size() - 1) {
                            ss << "AND ";
                        }
                        ss << ' ';
                    }
                    auto query = ss.str();
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                        statement_finalizer finalizer{stmt};
                        auto index = 1;
                        auto idsTuple = std::make_tuple(std::forward<Ids>(ids)...);
                        constexpr const auto idsCount = std::tuple_size<decltype(idsTuple)>::value;
                        tuple_helper::iterator<idsCount - 1, Ids...>()(idsTuple, [stmt, &index](auto &v){
                            typedef typename std::remove_reference<decltype(v)>::type field_type;
                            statement_binder<field_type>().bind(stmt, index++, v);
                        });
                        auto stepRes = sqlite3_step(stmt);
                        switch(stepRes){
                            case SQLITE_ROW:{
                                O res;
                                index = 0;
                                impl.table.for_each_column([&index, &res, stmt] (auto c) {
                                    typedef typename decltype(c)::field_type field_type;
                                    auto value = row_extractor<field_type>().extract(stmt, index++);
                                    if(c.member_pointer){
                                        res.*c.member_pointer = value;
                                    }else{
                                        ((res).*(c.setter))(std::move(value));
                                    }
                                });
                                return std::make_shared<O>(std::move(res));
                            }break;
                            case SQLITE_DONE:{
                                return {};
                            }break;
                            default:{
                                auto msg = sqlite3_errmsg(connection->get_db());
                                throw std::runtime_error(msg);
                            }
                        }
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("table " + impl.table.name + " has no primary key column");
                }
            }
            
            /**
             *  SELECT COUNT(*) with no conditions routine. https://www.sqlite.org/lang_aggfunc.html#count
             *  @return Number of O object in table.
             */
            template<class O, class ...Args>
            int count(Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                int res = 0;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::count_asterisk_t{}) << "(*) FROM '" << impl.table.name << "' ";
                this->process_conditions(ss, args...);
                auto query = ss.str();
                auto rc = sqlite3_exec(connection->get_db(),
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(int*)data;
                                           if(argc){
                                               res = row_extractor<int>().extract(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
            /**
             *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
             *  @param m member pointer to class mapped to the storage.
             */
            template<class F, class O, class ...Args>
            int count(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                int res = 0;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::count_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") FROM '"<< impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **) -> int {
                                               auto &res = *(int*)data;
                                               if(argc){
                                                   res = row_extractor<int>().extract(argv[0]);
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
            }
            
            /**
             *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return average value from db.
             */
            template<class F, class O, class ...Args>
            double avg(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                double res = 0;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::avg_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") FROM '"<< impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **)->int{
                                               auto &res = *(double*)data;
                                               if(argc){
                                                   res = row_extractor<double>().extract(argv[0]);
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
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
            typename sfinae = typename std::enable_if<std::tuple_size<std::tuple<Args...>>::value >= 1>::type
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
            std::string group_concat(F O::*m, const std::string &y, Args&& ...args) {
                return this->group_concat_internal(m, std::make_shared<std::string>(y), std::forward<Args>(args)...);
            }
            
            template<class F, class O, class ...Args>
            std::string group_concat(F O::*m, const char *y, Args&& ...args) {
                return this->group_concat_internal(m, std::make_shared<std::string>(y), std::forward<Args>(args)...);
            }
            
            /**
             *  MAX(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::shared_ptr with max value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args>
            std::shared_ptr<F> max(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::shared_ptr<F> res;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::max_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") FROM '" << impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **)->int{
                                               auto &res = *(std::shared_ptr<F>*)data;
                                               if(argc){
                                                   if(argv[0]){
                                                       res = std::make_shared<F>(row_extractor<F>().extract(argv[0]));
                                                   }
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
            }
            
            /**
             *  MIN(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::shared_ptr with min value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args>
            std::shared_ptr<F> min(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::shared_ptr<F> res;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::min_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") FROM '" << impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **)->int{
                                               auto &res = *(std::shared_ptr<F>*)data;
                                               if(argc){
                                                   if(argv[0]){
                                                       res = std::make_shared<F>(row_extractor<F>().extract(argv[0]));
                                                   }
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
            }
            
            /**
             *  SUM(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::shared_ptr with sum value or null if sqlite engine returned null.
             */
            template<class F, class O, class ...Args>
            std::shared_ptr<F> sum(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = this->get_impl<O>();
                std::shared_ptr<F> res;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::sum_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") FROM '"<< impl.table.name << "' ";
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **)->int{
                                               auto &res = *(std::shared_ptr<F>*)data;
                                               if(argc){
                                                   res = std::make_shared<F>(row_extractor<F>().extract(argv[0]));
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
            }
            
            /**
             *  TOTAL(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return total value (the same as SUM but not nullable. More details here https://www.sqlite.org/lang_aggfunc.html)
             */
            template<class F, class O, class ...Args>
            double total(F O::*m, Args&& ...args) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                double res;
                std::stringstream ss;
                ss << "SELECT " << static_cast<std::string>(aggregate_functions::total_t<int>{0}) << "(";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << ") ";
                    auto tableNamesSet = this->parse_table_names(m);
                    if(tableNamesSet.size()){
                        ss << "FROM " ;
                        std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                        for(size_t i = 0; i < tableNames.size(); ++i) {
                            ss << " '" << tableNames[i] << "' ";
                            if(i < tableNames.size() - 1) {
                                ss << ",";
                            }
                            ss << " ";
                        }
                    }
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    auto rc = sqlite3_exec(connection->get_db(),
                                           query.c_str(),
                                           [](void *data, int argc, char **argv,char **)->int{
                                               auto &res = *(double*)data;
                                               if(argc){
                                                   res = row_extractor<double>().extract(argv[0]);
                                               }
                                               return 0;
                                           }, &res, nullptr);
                    if(rc != SQLITE_OK) {
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
                return res;
            }
            
            /**
             *  Select a single column into std::vector<T>.
             */
            template<class T, class ...Args, class R = typename internal::column_result_t<T, Ts...>::type>
            std::vector<R> select(T m, Args&& ...args) {
                auto connection = this->get_or_create_connection();
                std::stringstream ss;
                ss << "SELECT ";
                auto columnName = this->string_from_expression(m);
                if(columnName.length()){
                    ss << columnName << " ";
                    auto tableNamesSet = this->parse_table_names(m);
                    if(tableNamesSet.size()){
                        ss << "FROM " ;
                        std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                        for(size_t i = 0; i < tableNames.size(); ++i) {
                            ss << " '" << tableNames[i] << "' ";
                            if(i < tableNames.size() - 1) {
                                ss << ",";
                            }
                            ss << " ";
                        }
                    }
                    this->process_conditions(ss, std::forward<Args>(args)...);
                    auto query = ss.str();
                    sqlite3_stmt *stmt;
                    if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                        statement_finalizer finalizer{stmt};
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
                                    auto msg = sqlite3_errmsg(connection->get_db());
                                    throw std::runtime_error(msg);
                                }
                            }
                        }while(stepRes != SQLITE_DONE);
                        return res;
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else{
                    throw std::runtime_error("column not found");
                }
            }
            
            /**
             *  Select several columns into std::vector<std::tuple<...>>.
             */
            template<class ...Args,
            class R = std::tuple<typename internal::column_result_t<Args, Ts...>::type...>,
            class ...Conds
            >
            std::vector<R> select(internal::columns_t<Args...> cols, Conds ...conds) {
                auto connection = this->get_or_create_connection();
                std::vector<R> res;
                std::stringstream ss;
                ss << "SELECT ";
                if(cols.distinct) {
                    ss << static_cast<std::string>(internal::distinct_t<int>{0}) << " ";
                }
                std::vector<std::string> columnNames;
                columnNames.reserve(cols.count());
                cols.for_each([&](auto &m) {
                    auto columnName = this->string_from_expression(m);
                    if(columnName.length()){
                        columnNames.push_back(columnName);
                    }else{
                        throw std::runtime_error("column not found");
                    }
                });
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                auto tableNamesSet = this->parse_table_names(cols);
                internal::join_iterator<Conds...>()([&](auto c){
                    typedef typename decltype(c)::type crossJoinType;
                    auto crossJoinedTableName = this->impl.template find_table_name<crossJoinType>();
                    tableNamesSet.erase(crossJoinedTableName);
                });
                if(tableNamesSet.size()){
                    ss << " FROM ";
                    std::vector<std::string> tableNames(tableNamesSet.begin(), tableNamesSet.end());
                    for(size_t i = 0; i < tableNames.size(); ++i) {
                        ss << " '" << tableNames[i] << "' ";
                        if(int(i) < int(tableNames.size()) - 1) {
                            ss << ",";
                        }
                        ss << " ";
                    }
                }
                this->process_conditions(ss, conds...);
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
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
                                auto msg = sqlite3_errmsg(connection->get_db());
                                throw std::runtime_error(msg);
                            }
                        }
                    }while(stepRes != SQLITE_DONE);
                    return res;
                }else{
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
            /**
             *  Returns a string representation of object of a class mapped to the storage.
             *  Type of string has json-like style.
             */
            template<class O>
            std::string dump(const O &o) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                return this->impl.dump(o, connection->get_db());
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
                
                auto connection = this->get_or_create_connection();
                auto &impl = get_impl<O>();
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
                ss << "VALUES(";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "?";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                        typedef typename decltype(c)::field_type field_type;
                        const field_type *value = nullptr;
                        if(c.member_pointer){
                            value = &(o.*c.member_pointer);
                        }else{
                            value = &((o).*(c.getter))();
                        }
                        statement_binder<field_type>().bind(stmt, index++, *value);
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //..
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            template<class It>
            void replace_range(It from, It to) {
                typedef typename std::iterator_traits<It>::value_type O;
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }
                
                auto connection = this->get_or_create_connection();
                auto &impl = get_impl<O>();
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
                auto valuesCount = static_cast<int>(std::distance(from, to));
                for(auto i = 0; i < valuesCount; ++i) {
                    ss << valuesString;
                    if(i < valuesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    for(auto it = from; it != to; ++it) {
                        auto &o = *it;
                        impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                            typedef typename decltype(c)::field_type field_type;
                            const field_type *value = nullptr;
                            if(c.member_pointer){
                                value = &(o.*c.member_pointer);
                            }else{
                                value = &((o).*(c.getter))();
                            }
                            statement_binder<field_type>().bind(stmt, index++, *value);
                        });
                    }
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //..
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            /**
             *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
             *  object doesn't matter.
             *  @return id of just created object.
             */
            template<class O>
            int insert(const O &o) {
                this->assert_mapped_type<O>();
                
                auto connection = this->get_or_create_connection();
                auto &impl = get_impl<O>();
                int res = 0;
                std::stringstream ss;
                ss << "INSERT INTO '" << impl.table.name << "' (";
                std::vector<std::string> columnNames;
                auto compositeKeyColumnNames = impl.table.composite_key_columns_names();
                impl.table.for_each_column([&impl, &columnNames, &compositeKeyColumnNames] (auto c) {
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
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "\"" << columnNames[i] << "\"";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ") ";
                    }
                }
                ss << "VALUES (";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << "?";
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << ")";
                    }
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    impl.table.for_each_column([&o, &index, &stmt, &impl, &compositeKeyColumnNames] (auto c) {
                        if(impl.table._without_rowid || !c.template has<constraints::primary_key_t<>>()){
                            auto it = std::find(compositeKeyColumnNames.begin(),
                                                compositeKeyColumnNames.end(),
                                                c.name);
                            if(it == compositeKeyColumnNames.end()){
                                typedef typename decltype(c)::field_type field_type;
                                const field_type *value = nullptr;
                                if(c.member_pointer){
                                    value = &(o.*c.member_pointer);
                                }else{
                                    value = &((o).*(c.getter))();
                                }
                                statement_binder<field_type>().bind(stmt, index++, *value);
                            }
                        }
                    });
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        res = int(sqlite3_last_insert_rowid(connection->get_db()));
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
            template<class It>
            void insert_range(It from, It to) {
                typedef typename std::iterator_traits<It>::value_type O;
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }
                
                auto connection = this->get_or_create_connection();
                auto &impl = get_impl<O>();
                
                std::stringstream ss;
                ss << "INSERT INTO '" << impl.table.name << "' (";
                std::vector<std::string> columnNames;
                impl.table.for_each_column([&] (auto c) {
                    if(!c.template has<constraints::primary_key_t<>>()) {
                        columnNames.emplace_back(c.name);
                    }
                });
                
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
                auto valuesCount = static_cast<int>(std::distance(from, to));
                for(auto i = 0; i < valuesCount; ++i) {
                    ss << valuesString;
                    if(i < valuesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    auto index = 1;
                    for(auto it = from; it != to; ++it) {
                        auto &o = *it;
                        impl.table.for_each_column([&o, &index, &stmt] (auto c) {
                            if(!c.template has<constraints::primary_key_t<>>()){
                                typedef typename decltype(c)::field_type field_type;
                                const field_type *value = nullptr;
                                if(c.member_pointer){
                                    value = &(o.*c.member_pointer);
                                }else{
                                    value = &((o).*(c.getter))();
                                }
                                statement_binder<field_type>().bind(stmt, index++, *value);
                            }
                        });
                    }
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //..
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
            void drop_index(const std::string &indexName) {
                auto connection = this->get_or_create_connection();
                std::stringstream ss;
                ss << "DROP INDEX '" << indexName + "'";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(connection->get_db(), query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(connection->get_db());
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
            }
            
        protected:
            
            void drop_table_internal(const std::string &tableName, sqlite3 *db) {
                std::stringstream ss;
                ss << "DROP TABLE '" << tableName + "'";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
        public:
            
            /**
             *  Drops table with given name.
             */
            void drop_table(const std::string &tableName) {
                auto connection = this->get_or_create_connection();
                this->drop_table_internal(tableName, connection->get_db());
            }
            
            /**
             *  sqlite3_changes function.
             */
            int changes() {
                auto connection = this->get_or_create_connection();
                return sqlite3_changes(connection->get_db());
            }
            
            /**
             *  sqlite3_total_changes function.
             */
            int total_changes() {
                auto connection = this->get_or_create_connection();
                return sqlite3_total_changes(connection->get_db());
            }
            
            int64 last_insert_rowid() {
                auto connection = this->get_or_create_connection();
                return sqlite3_last_insert_rowid(connection->get_db());
            }
            
            /**
             *  Returns libsqltie3 lib version, not sqlite_orm
             */
            std::string libversion() {
                return sqlite3_libversion();
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
                typedef typename decltype(impl->table)::columns_type columns_type;
                typedef typename std::tuple_element<0, columns_type>::type head_t;
                typedef typename internal::table_type<head_t>::type indexed_type;
                ss << "INDEX IF NOT EXISTS " << impl->table.name << " ON '" << this->impl.template find_table_name<indexed_type>() << "' ( ";
                std::vector<std::string> columnNames;
                tuple_helper::iterator<std::tuple_size<columns_type>::value - 1, Cols...>()(impl->table.columns, [&columnNames, this](auto &v){
                    columnNames.push_back(this->impl.column_name(v));
                });
                for(size_t i = 0; i < columnNames.size(); ++i) {
                    ss << columnNames[i];
                    if(i < columnNames.size() - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << ") ";
                auto query = ss.str();
                auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
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
                        if(schema_stat == sync_schema_result::old_columns_removed ||
                           schema_stat == sync_schema_result::new_columns_added ||
                           schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {
                            
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
                auto connection = this->get_or_create_connection();
                std::map<std::string, sync_schema_result> result;
                auto db = connection->get_db();
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
                auto connection = this->get_or_create_connection();
                std::map<std::string, sync_schema_result> result;
                auto db = connection->get_db();
                this->impl.for_each([&result, db, preserve](auto impl){
                    result.insert({impl->table.name, impl->schema_status(db, preserve)});
                });
                return result;
            }
            
            bool transaction(std::function<bool()> f) {
                this->begin_transaction();
                auto db = this->currentTransaction->get_db();
                auto shouldCommit = f();
                if(shouldCommit){
                    this->impl.commit(db);
                }else{
                    this->impl.rollback(db);
                }
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
                return shouldCommit;
            }
            
            void begin_transaction() {
                if(!this->inMemory){
                    if(!this->isOpenedForever){
                        if(this->currentTransaction) throw std::runtime_error("cannot start a transaction within a transaction");
                        this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                        this->on_open_internal(this->currentTransaction->get_db());
                    }
                }
                auto db = this->currentTransaction->get_db();
                this->impl.begin_transaction(db);
            }
            
            void commit() {
                if(!this->inMemory){
                    if(!this->currentTransaction) throw std::runtime_error("cannot commit - no transaction is active");
                }
                auto db = this->currentTransaction->get_db();
                this->impl.commit(db);
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
            }
            
            void rollback() {
                if(!this->inMemory){
                    if(!this->currentTransaction) throw std::runtime_error("cannot rollback - no transaction is active");
                }
                auto db = this->currentTransaction->get_db();
                this->impl.rollback(db);
                if(!this->inMemory && !this->isOpenedForever){
                    this->currentTransaction = nullptr;
                }
            }
            
            std::string current_timestamp() {
                auto connection = this->get_or_create_connection();
                return this->impl.current_timestamp(connection->get_db());
            }
            
        protected:
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            void foreign_keys(sqlite3 *db, bool value) {
                std::stringstream ss;
                ss << "PRAGMA foreign_keys = " << value;
                auto query = ss.str();
                auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            bool foreign_keys(sqlite3 *db) {
                std::string query = "PRAGMA foreign_keys";
                auto res = false;
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(bool*)data;
                                           if(argc){
                                               res = row_extractor<bool>().extract(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
#endif
            
        public:
            
#if SQLITE_VERSION_NUMBER >= 3007010
            /**
             * \fn db_release_memory
             * \brief Releases freeable memory of database. It is function can/should be called periodically by application,
             * if application has less memory usage constraint.
             * \note sqlite3_db_release_memory added in 3.7.10 https://sqlite.org/changes.html
             */
            int db_release_memory() {
                auto connection = this->get_or_create_connection();
                return sqlite3_db_release_memory(connection->get_db());
            }
#endif
            
            /**
             *  Checks whether table exists in db. Doesn't check storage itself - works only with actual database.
             *  Note: table can be not mapped to a storage
             *  @return true if table with a given name exists in db, false otherwise.
             */
            bool table_exists(const std::string &tableName) {
                auto connection = this->get_or_create_connection();
                return this->impl.table_exists(tableName, connection->get_db());
            }
            
            /**
             *  Returns existing permanent table names in database. Doesn't check storage itself - works only with actual database.
             *  @return Returns list of tables in database.
             */
            std::vector<std::string> table_names() {
                auto connection = this->get_or_create_connection();
                std::vector<std::string> tableNames;
                std::string sql = std::string("SELECT name FROM sqlite_master WHERE type='table'");
                typedef std::vector<std::string> Data;
                int res = sqlite3_exec(connection->get_db(), sql.c_str(),
                                       [] (void *data, int argc, char **argv, char **/*columnName*/) -> int {
                                           auto& tableNames = *(Data*)data;
                                           for(int i = 0; i < argc; i++) {
                                               if(argv[i]){
                                                   tableNames.push_back(argv[i]);
                                               }
                                           }
                                           return 0;
                                       }, &tableNames,nullptr);
                
                if(res != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(connection->get_db());
                    throw std::runtime_error(msg);
                }
                return tableNames;
            }
            
            void open_forever() {
                this->isOpenedForever = true;
                if(!this->currentTransaction){
                    this->currentTransaction = std::make_shared<internal::database_connection>(this->filename);
                    this->on_open_internal(this->currentTransaction->get_db());
                }
            }
            
            
        protected:
            std::string filename;
            impl_type impl;
            std::shared_ptr<internal::database_connection> currentTransaction;
            const bool inMemory;
            bool isOpenedForever = false;
        public:
            pragma_t pragma;
        };
    }
}

#endif /* storage_h */
