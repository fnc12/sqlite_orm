#pragma once

#include <memory>  //  std::unique/shared_ptr, std::make_unique/shared
#include <string>  //  std::string
#include <sqlite3.h>
#include <type_traits>  //  std::remove_reference, std::is_base_of, std::decay, std::false_type, std::true_type
#include <cstddef>  //  std::ptrdiff_t
#include <iterator>  //  std::input_iterator_tag, std::iterator_traits, std::distance
#include <functional>  //  std::function
#include <sstream>  //  std::stringstream
#include <map>  //  std::map
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple, std::make_tuple
#include <utility>  //  std::forward, std::pair
#include <algorithm>  //  std::find

#include <iostream>

using std::cout;
using std::endl;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

#include "alias.h"
#include "row_extractor_builder.h"
#include "error_code.h"
#include "type_printer.h"
#include "tuple_helper/tuple_helper.h"
#include "constraints.h"
#include "type_is_nullable.h"
#include "field_printer.h"
#include "rowid.h"
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
#include "expression_object_type.h"
#include "statement_serializator.h"
#include "table_name_collector.h"
#include "triggers.h"
#include "object_from_column_builder.h"
#include "table.h"
#include "column.h"
#include "index.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage`
         *  function.
         */
        template<class... Ts>
        struct storage_t : storage_base {
            using self = storage_t<Ts...>;
            using impl_type = storage_impl<Ts...>;

            /**
             *  @param filename database filename.
             *  @param impl_ storage_impl head
             */
            storage_t(const std::string& filename, impl_type impl_) :
                storage_base{filename, foreign_keys_count(impl_)}, impl(std::move(impl_)) {}

            storage_t(const storage_t& other) : storage_base(other), impl(other.impl) {}

          protected:
            impl_type impl;

            template<class T, class S, class... Args>
            friend struct view_t;

            template<class S>
            friend struct dynamic_order_by_t;

            template<class V>
            friend struct iterator_t;

            template<class S>
            friend struct serializator_context_builder;

            template<class I>
            void create_table(sqlite3* db, const std::string& tableName, const I& tableImpl) {
                using table_type = typename std::decay<decltype(tableImpl.table)>::type;
                std::stringstream ss;
                ss << "CREATE TABLE '" << tableName << "' ( ";
                auto elementsCount = tableImpl.table.elements_count;
                auto index = 0;
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                iterate_tuple(tableImpl.table.elements, [elementsCount, &index, &ss, &context](auto& element) {
                    ss << serialize(element, context);
                    if(index < elementsCount - 1) {
                        ss << ", ";
                    }
                    index++;
                });
                ss << ")";
                if(table_type::is_without_rowid) {
                    ss << " WITHOUT ROWID";
                }
                perform_void_exec(db, ss.str());
            }
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            void drop_column(sqlite3* db, const std::string& tableName, const std::string& columnName) {
                std::stringstream ss;
                ss << "ALTER TABLE '" << tableName << "' DROP COLUMN \"" << columnName << "\"";
                perform_void_exec(db, ss.str());
            }
#endif
            template<class I>
            void backup_table(sqlite3* db, const I& tableImpl, const std::vector<table_info*>& columnsToIgnore) {

                //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
                //  a name already exists we append suffix 1, then 2, etc until we find a free name..
                auto backupTableName = tableImpl.table.name + "_backup";
                if(tableImpl.table_exists(backupTableName, db)) {
                    int suffix = 1;
                    do {
                        std::stringstream stream;
                        stream << suffix;
                        auto anotherBackupTableName = backupTableName + stream.str();
                        if(!tableImpl.table_exists(anotherBackupTableName, db)) {
                            backupTableName = anotherBackupTableName;
                            break;
                        }
                        ++suffix;
                    } while(true);
                }

                this->create_table(db, backupTableName, tableImpl);

                tableImpl.copy_table(db, backupTableName, columnsToIgnore);

                this->drop_table_internal(tableImpl.table.name, db);

                tableImpl.rename_table(db, backupTableName, tableImpl.table.name);
            }

            template<class O>
            void assert_mapped_type() const {
                using mapped_types_tuples = std::tuple<typename Ts::object_type...>;
                static_assert(tuple_helper::has_type<O, mapped_types_tuples>::value, "type is not mapped to a storage");
            }

            template<class O>
            void assert_insertable_type() const {
                auto& tImpl = this->get_impl<O>();
                using table_type = typename std::decay<decltype(tImpl.table)>::type;
                using elements_type = typename std::decay<decltype(tImpl.table.elements)>::type;

                using is_without_rowid = std::integral_constant<bool, table_type::is_without_rowid>;

                static_if<is_without_rowid{}>(
                    [](auto&) {},  // all right. it's a "without_rowid" table
                    [](auto& tImpl) {  // unfortunately, this static_assert's can't see any composite keys((
                        std::ignore = tImpl;
                        static_assert(
                            count_tuple<elements_type, is_column_with_insertable_primary_key>::value <= 1,
                            "Attempting to execute 'insert' request into an noninsertable table was detected. "
                            "Insertable table cannot contain > 1 primary keys. Please use 'replace' instead of "
                            "'insert', or you can use 'insert' with explicit column listing.");
                        static_assert(
                            count_tuple<elements_type, is_column_with_noninsertable_primary_key>::value == 0,
                            "Attempting to execute 'insert' request into an noninsertable table was detected. "
                            "Insertable table cannot contain non-standard primary keys. Please use 'replace' instead "
                            "of 'insert', or you can use 'insert' with explicit column listing.");
                    })(tImpl);
            }

            template<class O>
            auto& get_impl() const {
                return this->impl.template get_impl<O>();
            }

            template<class O>
            auto& get_impl() {
                return this->impl.template get_impl<O>();
            }

          public:
            template<class T>
            void drop_trigger(const T& triggerName) {
                std::stringstream ss;
                ss << "DROP TRIGGER " << triggerName;
                auto query = ss.str();
                auto con = this->get_connection();
                auto db = con.get();
                perform_void_exec(db, query);
            }

            template<class T, class... Args>
            view_t<T, self, Args...> iterate(Args&&... args) {
                this->assert_mapped_type<T>();

                auto con = this->get_connection();
                return {*this, std::move(con), std::forward<Args>(args)...};
            }

            /**
             * Delete from routine.
             * O is an object's type. Must be specified explicitly.
             * @param args optional conditions: `where`, `join` etc
             * @example: storage.remove_all<User>(); - DELETE FROM users
             * @example: storage.remove_all<User>(where(in(&User::id, {5, 6, 7}))); - DELETE FROM users WHERE id IN (5, 6, 7)
             */
            template<class O, class... Args>
            void remove_all(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::remove_all<O>(std::forward<Args>(args)...));
                this->execute(statement);
            }

            /**
             *  Delete routine.
             *  O is an object's type. Must be specified explicitly.
             *  @param ids ids of object to be removed.
             */
            template<class O, class... Ids>
            void remove(Ids... ids) {
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
            void update(const O& o) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::update(std::ref(o)));
                this->execute(statement);
            }

            template<class... Args, class... Wargs>
            void update_all(internal::set_t<Args...> set, Wargs... wh) {
                auto statement = this->prepare(sqlite_orm::update_all(std::move(set), std::forward<Wargs>(wh)...));
                this->execute(statement);
            }

          protected:
            template<class F, class O, class... Args>
            std::string group_concat_internal(F O::*m, std::unique_ptr<std::string> y, Args&&... args) {
                this->assert_mapped_type<O>();
                std::vector<std::string> rows;
                if(y) {
                    rows = this->select(sqlite_orm::group_concat(m, move(*y)), std::forward<Args>(args)...);
                } else {
                    rows = this->select(sqlite_orm::group_concat(m), std::forward<Args>(args)...);
                }
                if(!rows.empty()) {
                    return move(rows.front());
                } else {
                    return {};
                }
            }

          public:
            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  @return All objects of type O stored in database at the moment in `std::vector`.
             *  @note If you need to return the result in a different container type then use a different `get_all` function overload `get_all<User, std::list<User>>`
             *  @example: storage.get_all<User>() - SELECT * FROM users
             *  @example: storage.get_all<User>(where(like(&User::name, "N%")), order_by(&User::id)); - SELECT * FROM users WHERE name LIKE 'N%' ORDER BY id
             */
            template<class O, class... Args>
            auto get_all(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all<O>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  R is an explicit return type. This type must have `push_back(O &&)` function.
             *  @return All objects of type O stored in database at the moment in `R`.
             *  @example: storage.get_all<User, std::list<User>>(); - SELECT * FROM users
             *  @example: storage.get_all<User, std::list<User>>(where(like(&User::name, "N%")), order_by(&User::id)); - SELECT * FROM users WHERE name LIKE 'N%' ORDER BY id
            */
            template<class O, class R, class... Args>
            auto get_all(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all<O, R>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  @return All objects of type O as `std::unique_ptr<O>` inside a `std::vector` stored in database at the moment.
             *  @note If you need to return the result in a different container type then use a different `get_all_pointer` function overload `get_all_pointer<User, std::list<User>>`
             *  @example: storage.get_all_pointer<User>(); - SELECT * FROM users
             *  @example: storage.get_all_pointer<User>(where(length(&User::name) > 6)); - SELECT * FROM users WHERE LENGTH(name)  > 6
             */
            template<class O, class... Args>
            auto get_all_pointer(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all_pointer<O>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  R is a container type. std::vector<std::unique_ptr<O>> is default
             *  @return All objects of type O as std::unique_ptr<O> stored in database at the moment.
             *  @example: storage.get_all_pointer<User, std::list<User>>(); - SELECT * FROM users
             *  @example: storage.get_all_pointer<User, std::list<User>>(where(length(&User::name) > 6)); - SELECT * FROM users WHERE LENGTH(name)  > 6
            */
            template<class O, class R, class... Args>
            auto get_all_pointer(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all_pointer<O, R>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

            /**
             *  Select * by id routine.
             *  throws std::system_error(orm_error_code::not_found, orm_error_category) if object not found with given
             * id. throws std::system_error with orm_error_category in case of db error. O is an object type to be
             * extracted. Must be specified explicitly.
             *  @return Object of type O where id is equal parameter passed or throws
             * `std::system_error(orm_error_code::not_found, orm_error_category)` if there is no object with such id.
             */
            template<class O, class... Ids>
            O get(Ids... ids) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get<O>(std::forward<Ids>(ids)...));
                return this->execute(statement);
            }

            /**
             *  The same as `get` function but doesn't throw an exception if noting found but returns std::unique_ptr
             * with null value. throws std::system_error in case of db error.
             */
            template<class O, class... Ids>
            std::unique_ptr<O> get_pointer(Ids... ids) {
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
            template<class O, class... Ids>
            std::shared_ptr<O> get_no_throw(Ids... ids) {
                return std::shared_ptr<O>(get_pointer<O>(std::forward<Ids>(ids)...));
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            /**
             *  The same as `get` function but doesn't throw an exception if noting found but
             * returns an empty std::optional. throws std::system_error in case of db error.
             */
            template<class O, class... Ids>
            std::optional<O> get_optional(Ids... ids) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_optional<O>(std::forward<Ids>(ids)...));
                return this->execute(statement);
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            /**
             *  SELECT COUNT(*) https://www.sqlite.org/lang_aggfunc.html#count
             *  @return Number of O object in table.
             */
            template<class O, class... Args, class R = typename mapped_type_proxy<O>::type>
            int count(Args&&... args) {
                this->assert_mapped_type<R>();
                auto rows = this->select(sqlite_orm::count<R>(), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return rows.front();
                } else {
                    return 0;
                }
            }

            /**
             *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
             *  @param m member pointer to class mapped to the storage.
             *  @return count of `m` values from database.
             */
            template<class F, class O, class... Args>
            int count(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::count(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return rows.front();
                } else {
                    return 0;
                }
            }

            /**
             *  AVG(X) query.   https://www.sqlite.org/lang_aggfunc.html#avg
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return average value from database.
             */
            template<class F, class O, class... Args>
            double avg(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::avg(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return rows.front();
                } else {
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
            template<class F,
                     class O,
                     class... Args,
                     class Tuple = std::tuple<Args...>,
                     typename sfinae = typename std::enable_if<std::tuple_size<Tuple>::value >= 1>::type>
            std::string group_concat(F O::*m, Args&&... args) {
                return this->group_concat_internal(m, {}, std::forward<Args>(args)...);
            }

            /**
             *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return group_concat query result.
             */
            template<class F, class O, class... Args>
            std::string group_concat(F O::*m, std::string y, Args&&... args) {
                return this->group_concat_internal(m,
                                                   std::make_unique<std::string>(move(y)),
                                                   std::forward<Args>(args)...);
            }

            template<class F, class O, class... Args>
            std::string group_concat(F O::*m, const char* y, Args&&... args) {
                std::unique_ptr<std::string> str;
                if(y) {
                    str = std::make_unique<std::string>(y);
                } else {
                    str = std::make_unique<std::string>();
                }
                return this->group_concat_internal(m, move(str), std::forward<Args>(args)...);
            }

            /**
             *  MAX(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with max value or null if sqlite engine returned null.
             */
            template<class F, class O, class... Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> max(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::max(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return std::move(rows.front());
                } else {
                    return {};
                }
            }

            /**
             *  MIN(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with min value or null if sqlite engine returned null.
             */
            template<class F, class O, class... Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> min(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::min(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return std::move(rows.front());
                } else {
                    return {};
                }
            }

            /**
             *  SUM(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with sum value or null if sqlite engine returned null.
             */
            template<class F, class O, class... Args, class Ret = typename column_result_t<self, F O::*>::type>
            std::unique_ptr<Ret> sum(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                std::vector<std::unique_ptr<double>> rows =
                    this->select(sqlite_orm::sum(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    if(rows.front()) {
                        return std::make_unique<Ret>(std::move(*rows.front()));
                    } else {
                        return {};
                    }
                } else {
                    return {};
                }
            }

            /**
             *  TOTAL(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return total value (the same as SUM but not nullable. More details here
             * https://www.sqlite.org/lang_aggfunc.html)
             */
            template<class F, class O, class... Args>
            double total(F O::*m, Args&&... args) {
                this->assert_mapped_type<O>();
                auto rows = this->select(sqlite_orm::total(m), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return std::move(rows.front());
                } else {
                    return {};
                }
            }

            /**
             *  Select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             *  For a single column use `auto rows = storage.select(&User::id, where(...));
             *  For multicolumns use `auto rows = storage.select(columns(&User::id, &User::name), where(...));
             */
            template<class T, class... Args, class R = typename column_result_t<self, T>::type>
            std::vector<R> select(T m, Args... args) {
                static_assert(!is_base_of_template<T, compound_operator>::value ||
                                  std::tuple_size<std::tuple<Args...>>::value == 0,
                              "Cannot use args with a compound operator");
                auto statement = this->prepare(sqlite_orm::select(std::move(m), std::forward<Args>(args)...));
                return this->execute(statement);
            }

            template<class T>
            typename std::enable_if<is_prepared_statement<T>::value, std::string>::type
            dump(const T& preparedStatement) const {
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                return serialize(preparedStatement.expression, context);
            }

            /**
             *  Returns a string representation of object of a class mapped to the storage.
             *  Type of string has json-like style.
             */
            template<class O>
            typename std::enable_if<storage_traits::type_is_mapped<self, O>::value, std::string>::type
            dump(const O& o) {
                auto& tImpl = this->get_impl<O>();
                std::stringstream ss;
                ss << "{ ";
                using pair = std::pair<std::string, std::string>;
                std::vector<pair> pairs;
                tImpl.table.for_each_column([&pairs, &o](auto& column) {
                    using column_type = typename std::decay<decltype(column)>::type;
                    using field_type = typename column_type::field_type;
                    pair p{column.name, std::string()};
                    if(column.member_pointer) {
                        p.second = field_printer<field_type>()(o.*column.member_pointer);
                    } else {
                        using getter_type = typename column_type::getter_type;
                        field_value_holder<getter_type> valueHolder{(o.*(column.getter))()};
                        p.second = field_printer<field_type>()(valueHolder.value);
                    }
                    pairs.push_back(move(p));
                });
                for(size_t i = 0; i < pairs.size(); ++i) {
                    auto& p = pairs[i];
                    ss << p.first << " : '" << p.second << "'";
                    if(i < pairs.size() - 1) {
                        ss << ", ";
                    } else {
                        ss << " }";
                    }
                }
                return ss.str();
            }

            /**
             *  This is REPLACE (INSERT OR REPLACE) function.
             *  Also if you need to insert value with knows id you should
             *  also you this function instead of insert cause inserts ignores
             *  id and creates own one.
             */
            template<class O>
            void replace(const O& o) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::replace(std::ref(o)));
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

            template<class T, class It, class L>
            void replace_range(It from, It to, L transformer) {
                this->assert_mapped_type<T>();
                if(from == to) {
                    return;
                }

                auto statement = this->prepare(sqlite_orm::replace_range<T>(from, to, std::move(transformer)));
                this->execute(statement);
            }

            template<class O, class... Cols>
            int insert(const O& o, columns_t<Cols...> cols) {
                constexpr const size_t colsCount = std::tuple_size<std::tuple<Cols...>>::value;
                static_assert(colsCount > 0, "Use insert or replace with 1 argument instead");
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(std::ref(o), std::move(cols)));
                return int(this->execute(statement));
            }

            /**
             *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
             *  object doesn't matter.
             *  @return id of just created object.
             */
            template<class O>
            int insert(const O& o) {
                this->assert_mapped_type<O>();
                this->assert_insertable_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(std::ref(o)));
                return int(this->execute(statement));
            }

            /**
             *  Raw insert routine. Use this if `insert` with object does not fit you. This insert is designed to be able
             *  to call any type of `INSERT` query with no limitations.
             *  @example
             *  ```sql
             *  INSERT INTO users (id, name) VALUES(5, 'Little Mix')
             *  ```
             *  will be
             *  ```c++
             *  storage.insert(into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix")));
             *  ```
             *  One more example:
             *  ```sql
             *  INSERT INTO singers (name) VALUES ('Sofia Reyes')('Kungs')
             *  ```
             *  will be
             *  ```c++
             *  storage.insert(into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs")));
             *  ```
             *  One can use `default_values` to add `DEFAULT VALUES` modifier:
             *  ```sql
             *  INSERT INTO users DEFAULT VALUES
             *  ```
             *  will be
             *  ```c++
             *  storage.insert(into<Singer>(), default_values());
             *  ```
             *  Also one can use `INSERT OR ABORT`/`INSERT OR FAIL`/`INSERT OR IGNORE`/`INSERT OR REPLACE`/`INSERT ROLLBACK`:
             *  ```c++
             *  storage.insert(or_ignore(), into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs")));
             *  storage.insert(or_rollback(), into<Singer>(), default_values());
             *  storage.insert(or_abort(), into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix")));
             *  ```
             */
            template<class... Args>
            void insert(Args... args) {
                auto statement = this->prepare(sqlite_orm::insert(std::forward<Args>(args)...));
                this->execute(statement);
            }

            /**
             *  Raw replace statement creation routine. Use this if `replace` with object does not fit you. This replace is designed to be able
             *  to call any type of `REPLACE` query with no limitations. Actually this is the same query as raw insert except `OR...` option existance.
             *  @example
             *  ```sql
             *  REPLACE INTO users (id, name) VALUES(5, 'Little Mix')
             *  ```
             *  will be
             *  ```c++
             *  storage.prepare(replace(into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix"))));
             *  ```
             *  One more example:
             *  ```sql
             *  REPLACE INTO singers (name) VALUES ('Sofia Reyes')('Kungs')
             *  ```
             *  will be
             *  ```c++
             *  storage.prepare(replace(into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs"))));
             *  ```
             *  One can use `default_values` to add `DEFAULT VALUES` modifier:
             *  ```sql
             *  REPLACE INTO users DEFAULT VALUES
             *  ```
             *  will be
             *  ```c++
             *  storage.prepare(replace(into<Singer>(), default_values()));
             *  ```
             */
            template<class... Args>
            void replace(Args... args) {
                auto statement = this->prepare(sqlite_orm::replace(std::forward<Args>(args)...));
                this->execute(statement);
            }

            template<class It>
            void insert_range(It from, It to) {
                using O = typename std::iterator_traits<It>::value_type;
                this->assert_mapped_type<O>();
                this->assert_insertable_type<O>();
                if(from == to) {
                    return;
                }
                auto statement = this->prepare(sqlite_orm::insert_range(from, to));
                this->execute(statement);
            }

            template<class T, class It, class L>
            void insert_range(It from, It to, L transformer) {
                this->assert_mapped_type<T>();
                this->assert_insertable_type<T>();
                if(from == to) {
                    return;
                }
                auto statement = this->prepare(sqlite_orm::insert_range<T>(from, to, std::move(transformer)));
                this->execute(statement);
            }

            /**
             * Change table name inside storage's schema info. This function does not
             * affect database
             */
            template<class O>
            void rename_table(std::string name) {
                this->assert_mapped_type<O>();
                auto& tImpl = this->get_impl<O>();
                tImpl.table.name = move(name);
            }

            using storage_base::rename_table;

            /**
             * Get table's name stored in storage's schema info. This function does not call
             * any SQLite queries
             */
            template<class O>
            const std::string& tablename() const {
                this->assert_mapped_type<O>();
                auto& tImpl = this->get_impl<O>();
                return tImpl.table.name;
            }

            template<class F, class O>
            const std::string* column_name(F O::*memberPointer) const {
                return this->impl.column_name(memberPointer);
            }

          protected:
            template<class... Tss, class... Cols>
            sync_schema_result schema_status(const storage_impl<index_t<Cols...>, Tss...>&, sqlite3*, bool) {
                return sync_schema_result::already_in_sync;
            }

            template<class T, bool WithoutRowId, class... Cs, class... Tss>
            sync_schema_result schema_status(const storage_impl<table_t<T, WithoutRowId, Cs...>, Tss...>& tImpl,
                                             sqlite3* db,
                                             bool preserve) {
                return tImpl.schema_status(db, preserve);
            }

            template<class... Tss, class... Cols>
            sync_schema_result sync_table(const storage_impl<index_t<Cols...>, Tss...>& tableImpl, sqlite3* db, bool) {
                auto res = sync_schema_result::already_in_sync;
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                auto query = serialize(tableImpl.table, context);
                perform_void_exec(db, query);
                return res;
            }

            template<class... Tss, class... Cols>
            sync_schema_result
            sync_table(const storage_impl<trigger_t<Cols...>, Tss...>& tableImpl, sqlite3* db, bool) {
                auto res = sync_schema_result::already_in_sync;  // TODO Change accordingly
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                // context.replace_bindable_with_question = true;
                auto query = serialize(tableImpl.table, context);
                auto rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return res;
            }

            template<class T, bool WithoutRowId, class... Args, class... Tss>
            sync_schema_result sync_table(const storage_impl<table_t<T, WithoutRowId, Args...>, Tss...>& tImpl,
                                          sqlite3* db,
                                          bool preserve) {
                auto res = sync_schema_result::already_in_sync;

                auto schema_stat = tImpl.schema_status(db, preserve);
                if(schema_stat != decltype(schema_stat)::already_in_sync) {
                    if(schema_stat == decltype(schema_stat)::new_table_created) {
                        this->create_table(db, tImpl.table.name, tImpl);
                        res = decltype(res)::new_table_created;
                    } else {
                        if(schema_stat == sync_schema_result::old_columns_removed ||
                           schema_stat == sync_schema_result::new_columns_added ||
                           schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {

                            //  get table info provided in `make_table` call..
                            auto storageTableInfo = tImpl.table.get_table_info();

                            //  now get current table info from db using `PRAGMA table_info` query..
                            auto dbTableInfo = tImpl.get_table_info(tImpl.table.name, db);

                            //  this vector will contain pointers to columns that gotta be added..
                            std::vector<table_info*> columnsToAdd;

                            tImpl.calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

                            if(schema_stat == sync_schema_result::old_columns_removed) {
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
                                for(auto& tableInfo: dbTableInfo) {
                                    this->drop_column(db, tImpl.table.name, tableInfo.name);
                                }
                                res = decltype(res)::old_columns_removed;
#else
                                //  extra table columns than storage columns
                                this->backup_table(db, tImpl, {});
                                res = decltype(res)::old_columns_removed;
#endif
                            }

                            if(schema_stat == sync_schema_result::new_columns_added) {
                                for(auto columnPointer: columnsToAdd) {
                                    tImpl.table.for_each_column([this, columnPointer, &tImpl, db](auto& column) {
                                        if(column.name != columnPointer->name) {
                                            return;
                                        }
                                        this->add_column(tImpl.table.name, column, db);
                                    });
                                }
                                res = decltype(res)::new_columns_added;
                            }

                            if(schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {

                                // remove extra columns
                                this->backup_table(db, tImpl, columnsToAdd);
                                res = decltype(res)::new_columns_added_and_old_columns_removed;
                            }
                        } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                            this->drop_table_internal(tImpl.table.name, db);
                            this->create_table(db, tImpl.table.name, tImpl);
                            res = decltype(res)::dropped_and_recreated;
                        }
                    }
                }
                return res;
            }

            template<class C>
            void add_column(const std::string& tableName, const C& column, sqlite3* db) const {
                std::stringstream ss;
                ss << "ALTER TABLE " << tableName << " ADD COLUMN ";
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                ss << serialize(column, context);
                perform_void_exec(db, ss.str());
            }

            template<typename S>
            prepared_statement_t<S> prepare_impl(S statement) {
                auto con = this->get_connection();
                sqlite3_stmt* stmt;
                auto db = con.get();
                using context_t = serializator_context<impl_type>;
                context_t context{this->impl};
                context.skip_table_name = false;
                context.replace_bindable_with_question = true;
                auto query = serialize(statement, context);
                if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    return prepared_statement_t<S>{std::forward<S>(statement), stmt, con};
                } else {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
            }

          public:
            /**
             *  This is a cute function used to replace migration up/down functionality.
             *  It performs check storage schema with actual db schema and:
             *  * if there are excess tables exist in db they are ignored (not dropped)
             *  * every table from storage is compared with it's db analog and
             *      * if table doesn't exist it is being created
             *      * if table exists its colums are being compared with table_info from db and
             *          * if there are columns in db that do not exist in storage (excess) table will be dropped and
             * recreated
             *          * if there are columns in storage that do not exist in db they will be added using `ALTER TABLE
             * ... ADD COLUMN ...' command
             *          * if there is any column existing in both db and storage but differs by any of
             * properties/constraints (pk, notnull, dflt_value) table will be dropped and recreated. Be aware that
             * `sync_schema` doesn't guarantee that data will not be dropped. It guarantees only that it will make db
             * schema the same as you specified in `make_storage` function call. A good point is that if you have no db
             * file at all it will be created and all tables also will be created with exact tables and columns you
             * specified in `make_storage`, `make_table` and `make_column` calls. The best practice is to call this
             * function right after storage creation.
             *  @param preserve affects function's behaviour in case it is needed to remove a column. If it is `false`
             * so table will be dropped if there is column to remove if SQLite version is < 3.35.0 and rmeove column if SQLite version >= 3.35.0,
             * if `true` -  table is being copied into another table, dropped and copied table is renamed with source table name.
             * Warning: sync_schema doesn't check foreign keys cause it is unable to do so in sqlite3. If you know how to get foreign key info please
             * submit an issue https://github.com/fnc12/sqlite_orm/issues
             *  @return std::map with std::string key equal table name and `sync_schema_result` as value.
             * `sync_schema_result` is a enum value that stores table state after syncing a schema. `sync_schema_result`
             * can be printed out on std::ostream with `operator<<`.
             */
            std::map<std::string, sync_schema_result> sync_schema(bool preserve = false) {
                auto con = this->get_connection();
                std::map<std::string, sync_schema_result> result;
                auto db = con.get();
                this->impl.for_each([&result, db, preserve, this](auto& storageImpl) {
                    auto res = this->sync_table(storageImpl, db, preserve);
                    result.insert({storageImpl.table.name, res});
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
                this->impl.for_each([&result, db, preserve, this](auto& tableImpl) {
                    auto schemaStatus = this->schema_status(tableImpl, db, preserve);
                    result.insert({tableImpl.table.name, schemaStatus});
                });
                return result;
            }

            /**
             *  Checks whether table exists in db. Doesn't check storage itself - works only with actual database.
             *  Note: table can be not mapped to a storage
             *  @return true if table with a given name exists in db, false otherwise.
             */
            bool table_exists(const std::string& tableName) {
                auto con = this->get_connection();
                return this->impl.table_exists(tableName, con.get());
            }

            template<class T, class... Args>
            prepared_statement_t<select_t<T, Args...>> prepare(select_t<T, Args...> sel) {
                sel.highest_level = true;
                return prepare_impl<select_t<T, Args...>>(std::move(sel));
            }

            template<class T, class... Args>
            prepared_statement_t<get_all_t<T, Args...>> prepare(get_all_t<T, Args...> get_) {
                return prepare_impl<get_all_t<T, Args...>>(std::move(get_));
            }

            template<class T, class... Args>
            prepared_statement_t<get_all_pointer_t<T, Args...>> prepare(get_all_pointer_t<T, Args...> get_) {
                return prepare_impl<get_all_pointer_t<T, Args...>>(std::move(get_));
            }

            template<class... Args>
            prepared_statement_t<replace_raw_t<Args...>> prepare(replace_raw_t<Args...> ins) {
                return prepare_impl<replace_raw_t<Args...>>(std::move(ins));
            }

            template<class... Args>
            prepared_statement_t<insert_raw_t<Args...>> prepare(insert_raw_t<Args...> ins) {
                return prepare_impl<insert_raw_t<Args...>>(std::move(ins));
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class R, class... Args>
            prepared_statement_t<get_all_optional_t<T, R, Args...>> prepare(get_all_optional_t<T, R, Args...> get_) {
                return prepare_impl<get_all_optional_t<T, R, Args...>>(std::move(get_));
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class... Args, class... Wargs>
            prepared_statement_t<update_all_t<set_t<Args...>, Wargs...>>
            prepare(update_all_t<set_t<Args...>, Wargs...> upd) {
                return prepare_impl<update_all_t<set_t<Args...>, Wargs...>>(std::move(upd));
            }

            template<class T, class... Args>
            prepared_statement_t<remove_all_t<T, Args...>> prepare(remove_all_t<T, Args...> rem) {
                return prepare_impl<remove_all_t<T, Args...>>(std::move(rem));
            }

            template<class T, class... Ids>
            prepared_statement_t<get_t<T, Ids...>> prepare(get_t<T, Ids...> get_) {
                return prepare_impl<get_t<T, Ids...>>(std::move(get_));
            }

            template<class T, class... Ids>
            prepared_statement_t<get_pointer_t<T, Ids...>> prepare(get_pointer_t<T, Ids...> get_) {
                return prepare_impl<get_pointer_t<T, Ids...>>(std::move(get_));
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class... Ids>
            prepared_statement_t<get_optional_t<T, Ids...>> prepare(get_optional_t<T, Ids...> get_) {
                return prepare_impl<get_optional_t<T, Ids...>>(std::move(get_));
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class T>
            prepared_statement_t<update_t<T>> prepare(update_t<T> upd) {
                return prepare_impl<update_t<T>>(std::move(upd));
            }

            template<class T, class... Ids>
            prepared_statement_t<remove_t<T, Ids...>> prepare(remove_t<T, Ids...> statement) {
                using object_type = typename expression_object_type<decltype(statement)>::type;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl<remove_t<T, Ids...>>(std::move(statement));
            }

            template<class T>
            prepared_statement_t<insert_t<T>> prepare(insert_t<T> statement) {
                using object_type = typename expression_object_type<decltype(statement)>::type;
                this->assert_mapped_type<object_type>();
                this->assert_insertable_type<object_type>();
                return this->prepare_impl<insert_t<T>>(std::move(statement));
            }

            template<class T>
            prepared_statement_t<replace_t<T>> prepare(replace_t<T> rep) {
                using object_type = typename expression_object_type<decltype(rep)>::type;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl<replace_t<T>>(std::move(rep));
            }

            template<class It, class L, class O>
            prepared_statement_t<insert_range_t<It, L, O>> prepare(insert_range_t<It, L, O> statement) {
                using object_type = typename expression_object_type<decltype(statement)>::type;
                this->assert_mapped_type<object_type>();
                this->assert_insertable_type<object_type>();
                return this->prepare_impl<insert_range_t<It, L, O>>(std::move(statement));
            }

            template<class It, class L, class O>
            prepared_statement_t<replace_range_t<It, L, O>> prepare(replace_range_t<It, L, O> statement) {
                using object_type = typename expression_object_type<decltype(statement)>::type;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl<replace_range_t<It, L, O>>(std::move(statement));
            }

            template<class T, class... Cols>
            prepared_statement_t<insert_explicit<T, Cols...>> prepare(insert_explicit<T, Cols...> ins) {
                using object_type = typename expression_object_type<decltype(ins)>::type;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl<insert_explicit<T, Cols...>>(std::move(ins));
            }

            template<class... Args>
            void execute(const prepared_statement_t<replace_raw_t<Args...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                sqlite3_reset(stmt);
                auto index = 1;
                iterate_ast(statement.expression.args, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                perform_step(db, stmt);
            }

            template<class... Args>
            void execute(const prepared_statement_t<insert_raw_t<Args...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                sqlite3_reset(stmt);
                auto index = 1;
                iterate_ast(statement.expression.args, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                perform_step(db, stmt);
            }

            template<class T, class... Cols>
            int64 execute(const prepared_statement_t<insert_explicit<T, Cols...>>& statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto& tImpl = this->get_impl<object_type>();
                auto& object = statement.expression.obj;
                sqlite3_reset(stmt);
                iterate_tuple(
                    statement.expression.columns.columns,
                    [&object, &index, &stmt, &tImpl, db](auto& memberPointer) {
                        using column_type = typename std::decay<decltype(memberPointer)>::type;
                        using field_type = typename column_result_t<self, column_type>::type;
                        const auto* value =
                            tImpl.table.template get_object_field_pointer<field_type>(object, memberPointer);
                        if(!value) {
                            throw std::system_error(std::make_error_code(sqlite_orm::orm_error_code::value_is_null));
                        }
                        if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, *value)) {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    });
                perform_step(db, stmt);
                return sqlite3_last_insert_rowid(db);
            }

            template<class T,
                     typename std::enable_if<(is_replace_range<T>::value || is_replace<T>::value)>::type* = nullptr>
            void execute(const prepared_statement_t<T>& statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto& tImpl = this->get_impl<object_type>();
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                sqlite3_reset(stmt);

                auto processObject = [&index, &stmt, &tImpl, db](auto& object) {
                    tImpl.table.for_each_column([&index, stmt, &object, db](auto& column) {
                        if(column.is_generated()) {
                            return;
                        }
                        using column_type = typename std::decay<decltype(column)>::type;
                        using field_type = typename column_type::field_type;
                        if(column.member_pointer) {
                            if(SQLITE_OK !=
                               statement_binder<field_type>().bind(stmt, index++, object.*column.member_pointer)) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        } else {
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((object).*(column.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        }
                    });
                };

                static_if<is_replace_range<T>{}>(
                    [&processObject](auto& statement) {
                        auto& transformer = statement.expression.transformer;
                        std::for_each(  ///
                            statement.expression.range.first,
                            statement.expression.range.second,
                            [&processObject, &transformer](auto& object) {
                                auto& realObject = transformer(object);
                                processObject(realObject);
                            });
                    },
                    [&processObject](auto& statement) {
                        auto& o = get_object(statement.expression);
                        processObject(o);
                    })(statement);
                perform_step(db, stmt);
            }

            template<class T,
                     typename std::enable_if<(is_insert_range<T>::value || is_insert<T>::value)>::type* = nullptr>
            int64 execute(const prepared_statement_t<T>& statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto index = 1;
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto& tImpl = this->get_impl<object_type>();
                sqlite3_reset(stmt);
                auto processObject = [&index, stmt, &tImpl, db](auto& object) {
                    tImpl.table.for_each_column([&tImpl, &index, &object, db, stmt](auto& column) {
                        using table_type = typename std::decay<decltype(tImpl.table)>::type;
                        if(table_type::is_without_rowid ||
                           (!column.template has<primary_key_t<>>() &&
                            !tImpl.table.exists_in_composite_primary_key(column) && !column.is_generated())) {
                            using column_type = typename std::decay<decltype(column)>::type;
                            using field_type = typename column_type::field_type;
                            if(column.member_pointer) {
                                if(SQLITE_OK !=
                                   statement_binder<field_type>().bind(stmt, index++, object.*column.member_pointer)) {
                                    throw std::system_error(
                                        std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
                                }
                            } else {
                                using getter_type = typename column_type::getter_type;
                                field_value_holder<getter_type> valueHolder{((object).*(column.getter))()};
                                if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)) {
                                    throw std::system_error(
                                        std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
                                }
                            }
                        }
                    });
                };

                static_if<is_insert_range<T>{}>(
                    [&processObject](auto& statement) {
                        auto& transformer = statement.expression.transformer;
                        std::for_each(  ///
                            statement.expression.range.first,
                            statement.expression.range.second,
                            [&processObject, &transformer](auto& object) {
                                auto& realObject = transformer(object);
                                processObject(realObject);
                            });
                    },
                    [&processObject](auto& statement) {
                        auto& o = get_object(statement.expression);
                        processObject(o);
                    })(statement);

                perform_step(db, stmt);
                return sqlite3_last_insert_rowid(db);
            }

            template<class T, class... Ids>
            void execute(const prepared_statement_t<remove_t<T, Ids...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression.ids, [stmt, &index, db](auto& v) {
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                perform_step(db, stmt);
            }

            template<class T>
            void execute(const prepared_statement_t<update_t<T>>& statement) {
                using statement_type = typename std::decay<decltype(statement)>::type;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;
                auto con = this->get_connection();
                auto db = con.get();
                auto& tImpl = this->get_impl<object_type>();
                auto stmt = statement.stmt;
                auto index = 1;
                auto& o = get_object(statement.expression);
                sqlite3_reset(stmt);
                tImpl.table.for_each_column([&o, stmt, &index, db, &tImpl](auto& column) {
                    if(!column.template has<primary_key_t<>>() &&
                       !tImpl.table.exists_in_composite_primary_key(column)) {
                        using column_type = typename std::decay<decltype(column)>::type;
                        using field_type = typename column_type::field_type;
                        if(column.member_pointer) {
                            auto bind_res =
                                statement_binder<field_type>().bind(stmt, index++, o.*column.member_pointer);
                            if(SQLITE_OK != bind_res) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        } else {
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((o).*(column.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        }
                    }
                });
                tImpl.table.for_each_column([&o, stmt, &index, db, &tImpl](auto& column) {
                    if(column.template has<primary_key_t<>>() || tImpl.table.exists_in_composite_primary_key(column)) {
                        using column_type = typename std::decay<decltype(column)>::type;
                        using field_type = typename column_type::field_type;
                        if(column.member_pointer) {
                            if(SQLITE_OK !=
                               statement_binder<field_type>().bind(stmt, index++, o.*column.member_pointer)) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        } else {
                            using getter_type = typename column_type::getter_type;
                            field_value_holder<getter_type> valueHolder{((o).*(column.getter))()};
                            if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, valueHolder.value)) {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        }
                    }
                });
                perform_step(db, stmt);
            }

            template<class T, class... Ids>
            std::unique_ptr<T> execute(const prepared_statement_t<get_pointer_t<T, Ids...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression.ids, [stmt, &index, db](auto& v) {
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes) {
                    case SQLITE_ROW: {
                        auto res = std::make_unique<T>();
                        object_from_column_builder<T> builder{*res, stmt};
                        tImpl.table.for_each_column(builder);
                        return res;
                    } break;
                    case SQLITE_DONE: {
                        return {};
                    } break;
                    default: {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                }
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class... Ids>
            std::optional<T> execute(const prepared_statement_t<get_optional_t<T, Ids...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression.ids, [stmt, &index, db](auto& v) {
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes) {
                    case SQLITE_ROW: {
                        auto res = std::make_optional<T>();
                        object_from_column_builder<T> builder{res.value(), stmt};
                        tImpl.table.for_each_column(builder);
                        return res;
                    } break;
                    case SQLITE_DONE: {
                        return {};
                    } break;
                    default: {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                }
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class T, class... Ids>
            T execute(const prepared_statement_t<get_t<T, Ids...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression.ids, [stmt, &index, db](auto& v) {
                    using field_type = typename std::decay<decltype(v)>::type;
                    if(SQLITE_OK != statement_binder<field_type>().bind(stmt, index++, v)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes) {
                    case SQLITE_ROW: {
                        T res;
                        object_from_column_builder<T> builder{res, stmt};
                        tImpl.table.for_each_column(builder);
                        return res;
                    } break;
                    case SQLITE_DONE: {
                        throw std::system_error(std::make_error_code(sqlite_orm::orm_error_code::not_found));
                    } break;
                    default: {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                }
            }

            template<class T, class... Args>
            void execute(const prepared_statement_t<remove_all_t<T, Args...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression.conditions, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                perform_step(db, stmt);
            }

            template<class... Args, class... Wargs>
            void execute(const prepared_statement_t<update_all_t<set_t<Args...>, Wargs...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_tuple(statement.expression.set.assigns, [&index, stmt, db](auto& setArg) {
                    iterate_ast(setArg, [&index, stmt, db](auto& node) {
                        using node_type = typename std::decay<decltype(node)>::type;
                        conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                        if(SQLITE_OK != binder(node)) {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    });
                });
                iterate_ast(statement.expression.conditions, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                perform_step(db, stmt);
            }

            template<class T, class... Args, class R = typename column_result_t<self, T>::type>
            std::vector<R> execute(const prepared_statement_t<select_t<T, Args...>>& statement) {
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                std::vector<R> res;
                auto tableInfoPointer = this->impl.template find_table<R>();
                int stepRes;
                do {
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes) {
                        case SQLITE_ROW: {
                            using table_info_pointer_t = typename std::remove_pointer<decltype(tableInfoPointer)>::type;
                            using table_info_t = typename std::decay<table_info_pointer_t>::type;
                            row_extractor_builder<R, storage_traits::type_is_mapped<self, R>::value, table_info_t>
                                builder;
                            auto rowExtractor = builder(tableInfoPointer);
                            res.push_back(rowExtractor.extract(stmt, 0));
                        } break;
                        case SQLITE_DONE:
                            break;
                        default: {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    }
                } while(stepRes != SQLITE_DONE);
                return res;
            }

            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_t<T, R, Args...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                R res;
                int stepRes;
                do {
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes) {
                        case SQLITE_ROW: {
                            T obj;
                            object_from_column_builder<T> builder{obj, stmt};
                            tImpl.table.for_each_column(builder);
                            res.push_back(std::move(obj));
                        } break;
                        case SQLITE_DONE:
                            break;
                        default: {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    }
                } while(stepRes != SQLITE_DONE);
                return res;
            }

            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_pointer_t<T, R, Args...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                R res;
                int stepRes;
                do {
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes) {
                        case SQLITE_ROW: {
                            auto obj = std::make_unique<T>();
                            object_from_column_builder<T> builder{*obj, stmt};
                            tImpl.table.for_each_column(builder);
                            res.push_back(move(obj));
                        } break;
                        case SQLITE_DONE:
                            break;
                        default: {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    }
                } while(stepRes != SQLITE_DONE);
                return res;
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_optional_t<T, R, Args...>>& statement) {
                auto& tImpl = this->get_impl<T>();
                auto con = this->get_connection();
                auto db = con.get();
                auto stmt = statement.stmt;
                auto index = 1;
                sqlite3_reset(stmt);
                iterate_ast(statement.expression, [stmt, &index, db](auto& node) {
                    using node_type = typename std::decay<decltype(node)>::type;
                    conditional_binder<node_type, is_bindable<node_type>> binder{stmt, index};
                    if(SQLITE_OK != binder(node)) {
                        throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                    }
                });
                R res;
                int stepRes;
                do {
                    stepRes = sqlite3_step(stmt);
                    switch(stepRes) {
                        case SQLITE_ROW: {
                            auto obj = std::make_optional<T>();
                            object_from_column_builder<T> builder{*obj, stmt};
                            tImpl.table.for_each_column(builder);
                            res.push_back(move(obj));
                        } break;
                        case SQLITE_DONE:
                            break;
                        default: {
                            throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                    sqlite3_errmsg(db));
                        }
                    }
                } while(stepRes != SQLITE_DONE);
                return res;
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class O>
            bool has_dependent_rows(const O& object) {
                auto res = false;
                this->impl.for_each([this, &object, &res](auto& storageImpl) {
                    if(res) {
                        return;
                    }
                    storageImpl.table.for_each_foreign_key([&storageImpl, this, &object, &res](auto& foreignKey) {
                        using ForeignKey = typename std::decay<decltype(foreignKey)>::type;
                        using TargetType = typename ForeignKey::target_type;
                        using SourceType = typename ForeignKey::source_type;
                        
                        static_if<std::is_same<TargetType, O>{}>([&storageImpl, this, &foreignKey, &res, &object]{
                            cout << "ForeignKey = " << SourceType::name() << " TargetType = " << TargetType::name() << endl;
                            std::stringstream ss;
                            ss << "SELECT COUNT(*)";
                            ss << " FROM " << storageImpl.table.name;
                            ss << " WHERE";
                            auto columnIndex = 0;
                            iterate_tuple(foreignKey.columns, [&ss, &columnIndex, &storageImpl](auto& column) {
                                if(columnIndex > 0) {
                                    ss << " AND";
                                }
                                if(auto columnName = storageImpl.table.find_column_name(column)) {
                                    ss << ' ' << *columnName << " = ?";
                                } else {
                                    throw std::system_error(
                                        std::make_error_code(sqlite_orm::orm_error_code::column_not_found));
                                }
                                ++columnIndex;
                            });
                            auto query = ss.str();
                            ss.flush();
                            auto con = this->get_connection();
                            sqlite3_stmt* stmt;
                            auto db = con.get();
                            if(sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                                statement_finalizer finalizer(stmt);
                                columnIndex = 1;
                                iterate_tuple(
                                    foreignKey.references,
                                    [&columnIndex, stmt, &object, db, this](auto memberPointer) {
                                        std::ignore = columnIndex;
                                        std::ignore = stmt;
                                        std::ignore = object;
                                        std::ignore = db;
                                        using MemberPointer = typename std::decay<decltype(memberPointer)>::type;
//                                        using field_type = typename member_traits<MemberPointer>::field_type;
                                        using object_type = typename member_traits<MemberPointer>::object_type;
                                        auto columnName = this->impl.column_name(memberPointer);
                                        std::ignore = columnName;
                                        cout << "reference object_type = " << object_type::name() << endl;
                                        using ObjectType = typename std::decay<decltype(object)>::type;
                                        auto objectTypesAreEqual = std::is_same<ObjectType, object_type>::value;
                                        cout << "objectTypesAreEqual = " << objectTypesAreEqual << endl;
                                        cout << "object::name = " << object.name() << endl;
//                                        auto objectCopy = object;
//                                        auto &nonConstObject = const_cast<O &>(object);
//                                        auto value = (objectCopy).*memberPointer;
//                                        std::ignore = value;
                                        /*if(SQLITE_OK != statement_binder<field_type>().bind(stmt,
                                                                                            columnIndex++,
                                                                                            object.*memberPointer)) {
                                            throw std::system_error(
                                                std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                                sqlite3_errmsg(db));
                                        }*/
                                    });
                                if(SQLITE_ROW != sqlite3_step(stmt)) {
                                    throw std::system_error(
                                        std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
                                }
                                auto countResult = sqlite3_column_int(stmt, 0);
                                res = countResult > 0;
                                if(SQLITE_DONE != sqlite3_step(stmt)) {
                                    throw std::system_error(
                                        std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                        sqlite3_errmsg(db));
                                }
                            } else {
                                throw std::system_error(
                                    std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                    sqlite3_errmsg(db));
                            }
                        })();
                    });
                });
                return res;
            }
        };  // struct storage_t

        template<class T>
        struct is_storage : std::false_type {};

        template<class... Ts>
        struct is_storage<storage_t<Ts...>> : std::true_type {};
    }

    template<class... Ts>
    internal::storage_t<Ts...> make_storage(const std::string& filename, Ts... tables) {
        return {filename, internal::storage_impl<Ts...>(std::forward<Ts>(tables)...)};
    }

    /**
     *  sqlite3_threadsafe() interface.
     */
    inline int threadsafe() {
        return sqlite3_threadsafe();
    }
}
