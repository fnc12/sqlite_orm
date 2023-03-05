#pragma once

#include <sqlite3.h>
#include <memory>  //  std::unique_ptr/shared_ptr, std::make_unique/std::make_shared
#include <system_error>  //  std::system_error
#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::is_base_of, std::decay, std::false_type, std::true_type
#include <functional>  //   std::identity
#include <sstream>  //  std::stringstream
#include <map>  //  std::map
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple, std::make_tuple, std::tie
#include <utility>  //  std::forward, std::pair
#include <algorithm>  //  std::for_each, std::ranges::for_each
#include "functional/cxx_optional.h"

#include "functional/cxx_universal.h"
#include "functional/cxx_functional_polyfill.h"
#include "functional/static_magic.h"
#include "functional/mpl.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "alias.h"
#include "row_extractor_builder.h"
#include "error_code.h"
#include "type_printer.h"
#include "constraints.h"
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
#include "view.h"
#include "ast_iterator.h"
#include "storage_base.h"
#include "prepared_statement.h"
#include "expression_object_type.h"
#include "statement_serializer.h"
#include "triggers.h"
#include "object_from_column_builder.h"
#include "table.h"
#include "column.h"
#include "index.h"
#include "util.h"
#include "serializing_util.h"

namespace sqlite_orm {

    namespace internal {

        template<class S, class E, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_preparable_v = false;

        template<class S, class E>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_preparable_v<S, E, polyfill::void_t<decltype(std::declval<S>().prepare(std::declval<E>()))>> = true;

        /**
         *  Storage class itself. Create an instanse to use it as an interfacto to sqlite db by calling `make_storage`
         *  function.
         */
        template<class... DBO>
        struct storage_t : storage_base {
            using self = storage_t<DBO...>;
            using db_objects_type = db_objects_tuple<DBO...>;

            /**
             *  @param filename database filename.
             *  @param dbObjects db_objects_tuple
             */
            storage_t(std::string filename, db_objects_type dbObjects) :
                storage_base{std::move(filename), foreign_keys_count(dbObjects)}, db_objects{std::move(dbObjects)} {}

          private:
            db_objects_type db_objects;

            /**
             *  Obtain a storage_t's const db_objects_tuple.
             *
             *  @note Historically, `serializer_context_builder` was declared friend, along with
             *  a few other library stock objects, in order to limit access to the db_objects_tuple.
             *  However, one could gain access to a storage_t's db_objects_tuple through
             *  `serializer_context_builder`, hence leading the whole friend declaration mambo-jumbo
             *  ad absurdum.
             *  Providing a free function is way better and cleaner.
             *
             *  Hence, friend was replaced by `obtain_db_objects()` and `pick_const_impl()`.
             */
            friend const db_objects_type& obtain_db_objects(const self& storage) noexcept {
                return storage.db_objects;
            }

            template<class Table>
            void create_table(sqlite3* db, const std::string& tableName, const Table& table) {
                using table_type = std::decay_t<decltype(table)>;
                using context_t = serializer_context<db_objects_type>;

                std::stringstream ss;
                context_t context{this->db_objects};
                ss << "CREATE TABLE " << streaming_identifier(tableName) << " ( "
                   << streaming_expressions_tuple(table.elements, context) << ")";
                if(table_type::is_without_rowid_v) {
                    ss << " WITHOUT ROWID";
                }
                ss.flush();
                perform_void_exec(db, ss.str());
            }

            /**
			*  Copies sourceTableName to another table with name: destinationTableName
			*  Performs INSERT INTO %destinationTableName% () SELECT %table.column_names% FROM %sourceTableName%
			*/
            template<class Table>
            void copy_table(sqlite3* db,
                            const std::string& sourceTableName,
                            const std::string& destinationTableName,
                            const Table& table,
                            const std::vector<const table_xinfo*>& columnsToIgnore) const;

#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            void drop_column(sqlite3* db, const std::string& tableName, const std::string& columnName) {
                std::stringstream ss;
                ss << "ALTER TABLE " << streaming_identifier(tableName) << " DROP COLUMN "
                   << streaming_identifier(columnName) << std::flush;
                perform_void_exec(db, ss.str());
            }
#endif

            template<class Table>
            void drop_create_with_loss(sqlite3* db, const Table& table) {
                // eliminated all transaction handling
                this->drop_table_internal(db, table.name);
                this->create_table(db, table.name, table);
            }

            template<class Table>
            void backup_table(sqlite3* db, const Table& table, const std::vector<const table_xinfo*>& columnsToIgnore) {

                //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
                //  a name already exists we append suffix 1, then 2, etc until we find a free name..
                auto backupTableName = table.name + "_backup";
                if(this->table_exists(db, backupTableName)) {
                    int suffix = 1;
                    do {
                        std::stringstream ss;
                        ss << suffix << std::flush;
                        auto anotherBackupTableName = backupTableName + ss.str();
                        if(!this->table_exists(db, anotherBackupTableName)) {
                            backupTableName = std::move(anotherBackupTableName);
                            break;
                        }
                        ++suffix;
                    } while(true);
                }
                this->create_table(db, backupTableName, table);

                this->copy_table(db, table.name, backupTableName, table, columnsToIgnore);

                this->drop_table_internal(db, table.name);

                this->rename_table(db, backupTableName, table.name);
            }

            template<class O>
            void assert_mapped_type() const {
                using mapped_types_tuple = std::tuple<typename DBO::object_type...>;
                static_assert(mpl::invoke_t<check_if_tuple_has_type<O>, mapped_types_tuple>::value,
                              "type is not mapped to a storage");
            }

            template<class O,
                     class Table = storage_pick_table_t<O, db_objects_type>,
                     std::enable_if_t<Table::is_without_rowid_v, bool> = true>
            void assert_insertable_type() const {}

            template<class O,
                     class Table = storage_pick_table_t<O, db_objects_type>,
                     std::enable_if_t<!Table::is_without_rowid_v, bool> = true>
            void assert_insertable_type() const {
                using elements_type = elements_type_t<Table>;
                using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;
                static_assert(
                    count_filtered_tuple<elements_type, is_primary_key_insertable, pkcol_index_sequence>::value <= 1,
                    "Attempting to execute 'insert' request into an noninsertable table was detected. "
                    "Insertable table cannot contain > 1 primary keys. Please use 'replace' instead of "
                    "'insert', or you can use 'insert' with explicit column listing.");
                static_assert(count_filtered_tuple<elements_type,
                                                   check_if_not<is_primary_key_insertable>::template fn,
                                                   pkcol_index_sequence>::value == 0,
                              "Attempting to execute 'insert' request into an noninsertable table was detected. "
                              "Insertable table cannot contain non-standard primary keys. Please use 'replace' instead "
                              "of 'insert', or you can use 'insert' with explicit column listing.");
            }

            template<class O>
            auto& get_table() const {
                return pick_table<O>(this->db_objects);
            }

            template<class O>
            auto& get_table() {
                return pick_table<O>(this->db_objects);
            }

          public:
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

            template<class S, class... Wargs>
            void update_all(S set, Wargs... wh) {
                static_assert(internal::is_set<S>::value,
                              "first argument in update_all can be either set or dynamic_set");
                auto statement = this->prepare(sqlite_orm::update_all(std::move(set), std::forward<Wargs>(wh)...));
                this->execute(statement);
            }

          protected:
            template<class F, class O, class... Args>
            std::string group_concat_internal(F O::*m, std::unique_ptr<std::string> y, Args&&... args) {
                this->assert_mapped_type<O>();
                std::vector<std::string> rows;
                if(y) {
                    rows = this->select(sqlite_orm::group_concat(m, std::move(*y)), std::forward<Args>(args)...);
                } else {
                    rows = this->select(sqlite_orm::group_concat(m), std::forward<Args>(args)...);
                }
                if(!rows.empty()) {
                    return std::move(rows.front());
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
             *  throws std::system_error{orm_error_code::not_found} if object not found with given
             * id. throws std::system_error with orm_error_category in case of db error. O is an object type to be
             * extracted. Must be specified explicitly.
             *  @return Object of type O where id is equal parameter passed or throws
             * `std::system_error{orm_error_code::not_found}` if there is no object with such id.
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
                return std::shared_ptr<O>(this->get_pointer<O>(std::forward<Ids>(ids)...));
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
            template<class O, class... Args, class R = mapped_type_proxy_t<O>>
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
                     std::enable_if_t<std::tuple_size<Tuple>::value >= 1, bool> = true>
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
                                                   std::make_unique<std::string>(std::move(y)),
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
                return this->group_concat_internal(m, std::move(str), std::forward<Args>(args)...);
            }

            /**
             *  MAX(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with max value or null if sqlite engine returned null.
             */
            template<class F, class O, class... Args, class Ret = column_result_of_t<db_objects_type, F O::*>>
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
            template<class F, class O, class... Args, class Ret = column_result_of_t<db_objects_type, F O::*>>
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
            template<class F, class O, class... Args, class Ret = column_result_of_t<db_objects_type, F O::*>>
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
            template<class T, class... Args, class R = column_result_of_t<db_objects_type, T>>
            std::vector<R> select(T m, Args... args) {
                static_assert(!is_compound_operator_v<T> || sizeof...(Args) == 0,
                              "Cannot use args with a compound operator");
                auto statement = this->prepare(sqlite_orm::select(std::move(m), std::forward<Args>(args)...));
                return this->execute(statement);
            }

            template<class T, satisfies<is_prepared_statement, T> = true>
            std::string dump(const T& preparedStatement, bool parametrized = true) const {
                return this->dump(preparedStatement.expression, parametrized);
            }

            template<class E,
                     class Ex = polyfill::remove_cvref_t<E>,
                     std::enable_if_t<!is_prepared_statement_v<Ex> && !is_mapped_v<db_objects_type, Ex>, bool> = true>
            std::string dump(E&& expression, bool parametrized = false) const {
                static_assert(is_preparable_v<self, Ex>, "Expression must be a high-level statement");

                decltype(auto) e2 = static_if<is_select_v<Ex>>(
                    [](auto expression) -> auto{
                        expression.highest_level = true;
                        return expression;
                    },
                    [](const auto& expression) -> decltype(auto) {
                        return (expression);
                    })(std::forward<E>(expression));
                using context_t = serializer_context<db_objects_type>;
                context_t context{this->db_objects};
                context.replace_bindable_with_question = parametrized;
                // just like prepare_impl()
                context.skip_table_name = false;
                return serialize(e2, context);
            }

            /**
             *  Returns a string representation of object of a class mapped to the storage.
             *  Type of string has json-like style.
             */
            template<class O, satisfies<is_mapped, db_objects_type, O> = true>
            std::string dump(const O& object) const {
                auto& table = this->get_table<O>();
                std::stringstream ss;
                ss << "{ ";
                table.for_each_column([&ss, &object, first = true](auto& column) mutable {
                    using column_type = std::decay_t<decltype(column)>;
                    using field_type = typename column_type::field_type;
                    constexpr std::array<const char*, 2> sep = {", ", ""};

                    ss << sep[std::exchange(first, false)] << column.name << " : '"
                       << field_printer<field_type>{}(polyfill::invoke(column.member_pointer, object)) << "'";
                });
                ss << " }";
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

            template<class It, class Projection = polyfill::identity>
            void replace_range(It from, It to, Projection project = {}) {
                using O = std::decay_t<decltype(polyfill::invoke(std::declval<Projection>(), *std::declval<It>()))>;
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }

                auto statement =
                    this->prepare(sqlite_orm::replace_range(std::move(from), std::move(to), std::move(project)));
                this->execute(statement);
            }

            template<class O, class It, class Projection = polyfill::identity>
            void replace_range(It from, It to, Projection project = {}) {
                this->assert_mapped_type<O>();
                if(from == to) {
                    return;
                }

                auto statement =
                    this->prepare(sqlite_orm::replace_range<O>(std::move(from), std::move(to), std::move(project)));
                this->execute(statement);
            }

            template<class O, class... Cols>
            int insert(const O& o, columns_t<Cols...> cols) {
                static_assert(cols.count > 0, "Use insert or replace with 1 argument instead");
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

            template<class It, class Projection = polyfill::identity>
            void insert_range(It from, It to, Projection project = {}) {
                using O = std::decay_t<decltype(polyfill::invoke(std::declval<Projection>(), *std::declval<It>()))>;
                this->assert_mapped_type<O>();
                this->assert_insertable_type<O>();
                if(from == to) {
                    return;
                }
                auto statement =
                    this->prepare(sqlite_orm::insert_range(std::move(from), std::move(to), std::move(project)));
                this->execute(statement);
            }

            template<class O, class It, class Projection = polyfill::identity>
            void insert_range(It from, It to, Projection project = {}) {
                this->assert_mapped_type<O>();
                this->assert_insertable_type<O>();
                if(from == to) {
                    return;
                }
                auto statement =
                    this->prepare(sqlite_orm::insert_range<O>(std::move(from), std::move(to), std::move(project)));
                this->execute(statement);
            }

            /**
             * Change table name inside storage's schema info. This function does not
             * affect database
             */
            template<class O>
            void rename_table(std::string name) {
                this->assert_mapped_type<O>();
                auto& table = this->get_table<O>();
                table.name = std::move(name);
            }

            using storage_base::rename_table;

            /**
             * Get table's name stored in storage's schema info. This function does not call
             * any SQLite queries
             */
            template<class O>
            const std::string& tablename() const {
                this->assert_mapped_type<O>();
                auto& table = this->get_table<O>();
                return table.name;
            }

            template<class F, class O>
            [[deprecated("Use the more accurately named function `find_column_name()`")]] const std::string*
            column_name(F O::*memberPointer) const {
                return internal::find_column_name(this->db_objects, memberPointer);
            }

            template<class F, class O>
            const std::string* find_column_name(F O::*memberPointer) const {
                return internal::find_column_name(this->db_objects, memberPointer);
            }

          protected:
            template<class... Cols>
            sync_schema_result schema_status(const index_t<Cols...>&, sqlite3*, bool, bool*) {
                return sync_schema_result::already_in_sync;
            }

            template<class T, bool WithoutRowId, class... Cs>
            sync_schema_result schema_status(const table_t<T, WithoutRowId, Cs...>& table,
                                             sqlite3* db,
                                             bool preserve,
                                             bool* attempt_to_preserve) {
                if(attempt_to_preserve) {
                    *attempt_to_preserve = true;
                }

                auto dbTableInfo = this->pragma.table_xinfo(table.name);
                auto res = sync_schema_result::already_in_sync;

                //  first let's see if table with such name exists..
                auto gottaCreateTable = !this->table_exists(db, table.name);
                if(!gottaCreateTable) {

                    //  get table info provided in `make_table` call..
                    auto storageTableInfo = table.get_table_info();

                    //  this vector will contain pointers to columns that gotta be added..
                    std::vector<const table_xinfo*> columnsToAdd;

                    if(calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo)) {
                        gottaCreateTable = true;
                    }

                    if(!gottaCreateTable) {  //  if all storage columns are equal to actual db columns but there are
                        //  excess columns at the db..
                        if(!dbTableInfo.empty()) {
                            // extra table columns than storage columns
                            if(!preserve) {
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
                                res = sync_schema_result::old_columns_removed;
#else
                                gottaCreateTable = true;
#endif
                            } else {
                                res = sync_schema_result::old_columns_removed;
                            }
                        }
                    }
                    if(gottaCreateTable) {
                        res = sync_schema_result::dropped_and_recreated;
                    } else {
                        if(!columnsToAdd.empty()) {
                            // extra storage columns than table columns
                            for(const table_xinfo* colInfo: columnsToAdd) {
                                const basic_generated_always::storage_type* generatedStorageType =
                                    table.find_column_generated_storage_type(colInfo->name);
                                if(generatedStorageType) {
                                    if(*generatedStorageType == basic_generated_always::storage_type::stored) {
                                        gottaCreateTable = true;
                                        break;
                                    }
                                    //  fallback cause VIRTUAL can be added
                                } else {
                                    if(colInfo->notnull && colInfo->dflt_value.empty()) {
                                        gottaCreateTable = true;
                                        // no matter if preserve is true or false, there is no way to preserve data, so we wont try!
                                        if(attempt_to_preserve) {
                                            *attempt_to_preserve = false;
                                        };
                                        break;
                                    }
                                }
                            }
                            if(!gottaCreateTable) {
                                if(res == sync_schema_result::old_columns_removed) {
                                    res = sync_schema_result::new_columns_added_and_old_columns_removed;
                                } else {
                                    res = sync_schema_result::new_columns_added;
                                }
                            } else {
                                res = sync_schema_result::dropped_and_recreated;
                            }
                        } else {
                            if(res != sync_schema_result::old_columns_removed) {
                                res = sync_schema_result::already_in_sync;
                            }
                        }
                    }
                } else {
                    res = sync_schema_result::new_table_created;
                }
                return res;
            }

            template<class... Cols>
            sync_schema_result sync_table(const index_t<Cols...>& index, sqlite3* db, bool) {
                auto res = sync_schema_result::already_in_sync;
                using context_t = serializer_context<db_objects_type>;
                context_t context{this->db_objects};
                auto query = serialize(index, context);
                perform_void_exec(db, query);
                return res;
            }

            template<class... Cols>
            sync_schema_result sync_table(const trigger_t<Cols...>& trigger, sqlite3* db, bool) {
                auto res = sync_schema_result::already_in_sync;  // TODO Change accordingly
                using context_t = serializer_context<db_objects_type>;
                context_t context{this->db_objects};
                perform_void_exec(db, serialize(trigger, context));
                return res;
            }

            template<class Table, satisfies<is_table, Table> = true>
            sync_schema_result sync_table(const Table& table, sqlite3* db, bool preserve);

            template<class C>
            void add_column(sqlite3* db, const std::string& tableName, const C& column) const {
                using context_t = serializer_context<db_objects_type>;

                context_t context{this->db_objects};
                std::stringstream ss;
                ss << "ALTER TABLE " << streaming_identifier(tableName) << " ADD COLUMN " << serialize(column, context)
                   << std::flush;
                perform_void_exec(db, ss.str());
            }

            template<typename S>
            prepared_statement_t<S> prepare_impl(S statement) {
                using context_t = serializer_context<db_objects_type>;
                context_t context{this->db_objects};
                context.skip_table_name = false;
                context.replace_bindable_with_question = true;

                auto con = this->get_connection();
                sqlite3_stmt* stmt = prepare_stmt(con.get(), serialize(statement, context));
                return prepared_statement_t<S>{std::forward<S>(statement), stmt, con};
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
             * so table will be dropped if there is column to remove if SQLite version is < 3.35.0 and remove column if SQLite version >= 3.35.0,
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
                iterate_tuple<true>(this->db_objects, [this, db = con.get(), preserve, &result](auto& schemaObject) {
                    sync_schema_result status = this->sync_table(schemaObject, db, preserve);
                    result.emplace(schemaObject.name, status);
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
                iterate_tuple<true>(this->db_objects, [this, db = con.get(), preserve, &result](auto& schemaObject) {
                    sync_schema_result status = this->schema_status(schemaObject, db, preserve, nullptr);
                    result.emplace(schemaObject.name, status);
                });
                return result;
            }

            using storage_base::table_exists;  // now that it is in storage_base make it into overload set

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

            template<class S, class... Wargs>
            prepared_statement_t<update_all_t<S, Wargs...>> prepare(update_all_t<S, Wargs...> upd) {
                return prepare_impl<update_all_t<S, Wargs...>>(std::move(upd));
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
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression.args, conditional_binder{statement.stmt});
                perform_step(stmt);
            }

            template<class... Args>
            void execute(const prepared_statement_t<insert_raw_t<Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression.args, conditional_binder{stmt});
                perform_step(stmt);
            }

            template<class T, class... Cols>
            int64 execute(const prepared_statement_t<insert_explicit<T, Cols...>>& statement) {
                using statement_type = std::decay_t<decltype(statement)>;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                tuple_value_binder{stmt}(
                    statement.expression.columns.columns,
                    [&table = this->get_table<object_type>(), &object = statement.expression.obj](auto& memberPointer) {
                        return table.object_field_value(object, memberPointer);
                    });
                perform_step(stmt);
                return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt));
            }

            template<class T,
                     std::enable_if_t<polyfill::disjunction_v<is_replace<T>, is_replace_range<T>>, bool> = true>
            void execute(const prepared_statement_t<T>& statement) {
                using statement_type = std::decay_t<decltype(statement)>;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                auto processObject = [&table = this->get_table<object_type>(),
                                      bindValue = field_value_binder{stmt}](auto& object) mutable {
                    table.template for_each_column_excluding<is_generated_always>(
                        call_as_template_base<column_field>([&bindValue, &object](auto& column) {
                            bindValue(polyfill::invoke(column.member_pointer, object));
                        }));
                };

                static_if<is_replace_range_v<T>>(
                    [&processObject](auto& expression) {
#if __cpp_lib_ranges >= 201911L
                        std::ranges::for_each(expression.range.first,
                                              expression.range.second,
                                              std::ref(processObject),
                                              std::ref(expression.transformer));
#else
                        auto& transformer = expression.transformer;
                        std::for_each(expression.range.first,
                                      expression.range.second,
                                      [&processObject, &transformer](auto& item) {
                                          const object_type& object = polyfill::invoke(transformer, item);
                                          processObject(object);
                                      });
#endif
                    },
                    [&processObject](auto& expression) {
                        const object_type& o = get_object(expression);
                        processObject(o);
                    })(statement.expression);

                perform_step(stmt);
            }

            template<class T, std::enable_if_t<polyfill::disjunction_v<is_insert<T>, is_insert_range<T>>, bool> = true>
            int64 execute(const prepared_statement_t<T>& statement) {
                using statement_type = std::decay_t<decltype(statement)>;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                auto processObject = [&table = this->get_table<object_type>(),
                                      bindValue = field_value_binder{stmt}](auto& object) mutable {
                    using is_without_rowid = typename std::decay_t<decltype(table)>::is_without_rowid;
                    table.template for_each_column_excluding<
                        mpl::conjunction<mpl::not_<mpl::always<is_without_rowid>>,
                                         mpl::disjunction_fn<is_primary_key, is_generated_always>>>(
                        call_as_template_base<column_field>([&table, &bindValue, &object](auto& column) {
                            if(!table.exists_in_composite_primary_key(column)) {
                                bindValue(polyfill::invoke(column.member_pointer, object));
                            }
                        }));
                };

                static_if<is_insert_range_v<T>>(
                    [&processObject](auto& expression) {
#if __cpp_lib_ranges >= 201911L
                        std::ranges::for_each(expression.range.first,
                                              expression.range.second,
                                              std::ref(processObject),
                                              std::ref(expression.transformer));
#else
                        auto& transformer = expression.transformer;
                        std::for_each(expression.range.first,
                                      expression.range.second,
                                      [&processObject, &transformer](auto& item) {
                                          const object_type& object = polyfill::invoke(transformer, item);
                                          processObject(object);
                                      });
#endif
                    },
                    [&processObject](auto& expression) {
                        const object_type& o = get_object(expression);
                        processObject(o);
                    })(statement.expression);

                perform_step(stmt);
                return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt));
            }

            template<class T, class... Ids>
            void execute(const prepared_statement_t<remove_t<T, Ids...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression.ids, conditional_binder{stmt});
                perform_step(stmt);
            }

            template<class T>
            void execute(const prepared_statement_t<update_t<T>>& statement) {
                using statement_type = std::decay_t<decltype(statement)>;
                using expression_type = typename statement_type::expression_type;
                using object_type = typename expression_object_type<expression_type>::type;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                auto& table = this->get_table<object_type>();

                field_value_binder bindValue{stmt};
                auto& object = get_object(statement.expression);
                table.template for_each_column_excluding<mpl::disjunction_fn<is_primary_key, is_generated_always>>(
                    call_as_template_base<column_field>([&table, &bindValue, &object](auto& column) {
                        if(!table.exists_in_composite_primary_key(column)) {
                            bindValue(polyfill::invoke(column.member_pointer, object));
                        }
                    }));
                table.for_each_column([&table, &bindValue, &object](auto& column) {
                    if(column.template is<is_primary_key>() || table.exists_in_composite_primary_key(column)) {
                        bindValue(polyfill::invoke(column.member_pointer, object));
                    }
                });
                perform_step(stmt);
            }

            template<class T, class... Ids>
            std::unique_ptr<T> execute(const prepared_statement_t<get_pointer_t<T, Ids...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression.ids, conditional_binder{stmt});

                std::unique_ptr<T> res;
                perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    res = std::make_unique<T>();
                    object_from_column_builder<T> builder{*res, stmt};
                    table.for_each_column(builder);
                });
                return res;
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class... Ids>
            std::optional<T> execute(const prepared_statement_t<get_optional_t<T, Ids...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression.ids, conditional_binder{stmt});

                std::optional<T> res;
                perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    object_from_column_builder<T> builder{res.emplace(), stmt};
                    table.for_each_column(builder);
                });
                return res;
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class T, class... Ids>
            T execute(const prepared_statement_t<get_t<T, Ids...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression.ids, conditional_binder{stmt});

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                std::optional<T> res;
                perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    object_from_column_builder<T> builder{res.emplace(), stmt};
                    table.for_each_column(builder);
                });
                if(!res.has_value()) {
                    throw std::system_error{orm_error_code::not_found};
                }
                return std::move(res).value();
#else
                auto& table = this->get_table<T>();
                auto stepRes = sqlite3_step(stmt);
                switch(stepRes) {
                    case SQLITE_ROW: {
                        T res;
                        object_from_column_builder<T> builder{res, stmt};
                        table.for_each_column(builder);
                        return res;
                    } break;
                    case SQLITE_DONE: {
                        throw std::system_error{orm_error_code::not_found};
                    } break;
                    default: {
                        throw_translated_sqlite_error(stmt);
                    }
                }
#endif
            }

            template<class T, class... Args>
            void execute(const prepared_statement_t<remove_all_t<T, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression.conditions, conditional_binder{stmt});
                perform_step(stmt);
            }

            template<class S, class... Wargs>
            void execute(const prepared_statement_t<update_all_t<S, Wargs...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                conditional_binder bindNode{stmt};
                iterate_ast(statement.expression.set, bindNode);
                iterate_ast(statement.expression.conditions, bindNode);
                perform_step(stmt);
            }

            template<class T, class... Args, class R = column_result_of_t<db_objects_type, T>>
            std::vector<R> execute(const prepared_statement_t<select_t<T, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                std::vector<R> res;
                perform_steps(stmt,
                              [rowExtractor = make_row_extractor<R>(lookup_table<R>(this->db_objects)),
                               &res](sqlite3_stmt* stmt) {
                                  res.push_back(rowExtractor.extract(stmt, 0));
                              });
                return res;
            }

            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_t<T, R, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                R res;
                perform_steps(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    T obj;
                    object_from_column_builder<T> builder{obj, stmt};
                    table.for_each_column(builder);
                    res.push_back(std::move(obj));
                });
                return res;
            }

            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_pointer_t<T, R, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                R res;
                perform_steps(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    auto obj = std::make_unique<T>();
                    object_from_column_builder<T> builder{*obj, stmt};
                    table.for_each_column(builder);
                    res.push_back(std::move(obj));
                });
                return res;
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class R, class... Args>
            R execute(const prepared_statement_t<get_all_optional_t<T, R, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                R res;
                perform_steps(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                    auto obj = std::make_optional<T>();
                    object_from_column_builder<T> builder{*obj, stmt};
                    table.for_each_column(builder);
                    res.push_back(std::move(obj));
                });
                return res;
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
        };  // struct storage_t
    }

    /*
     *  Factory function for a storage, from a database file and a bunch of database object definitions.
     */
    template<class... DBO>
    internal::storage_t<DBO...> make_storage(std::string filename, DBO... dbObjects) {
        return {std::move(filename), internal::db_objects_tuple<DBO...>{std::forward<DBO>(dbObjects)...}};
    }

    /**
     *  sqlite3_threadsafe() interface.
     */
    inline int threadsafe() {
        return sqlite3_threadsafe();
    }
}
