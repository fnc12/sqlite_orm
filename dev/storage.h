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
#include "tuple_helper/tuple_transformer.h"
#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "alias.h"
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
#include "mapped_view.h"
#include "result_set_view.h"
#include "ast_iterator.h"
#include "storage_base.h"
#include "prepared_statement.h"
#include "expression_object_type.h"
#include "statement_serializer.h"
#include "serializer_context.h"
#include "schema/triggers.h"
#include "object_from_column_builder.h"
#include "row_extractor.h"
#include "schema/table.h"
#include "schema/column.h"
#include "schema/index.h"
#include "cte_storage.h"
#include "util.h"
#include "serializing_util.h"

#ifdef SQLITE_ORM_INSERT_RETURN_TYPE_INT64
#define SQLITE_ORM_INSERT_RETURN_TYPE int64
#else
#define SQLITE_ORM_INSERT_RETURN_TYPE int
#endif

namespace sqlite_orm {

    namespace internal {
        /*
         *  Implementation note: the technique of indirect expression testing is because
         *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
         *  It must also be a type that differs from those for `is_printable_v`, `is_bindable_v`.
         */
        template<class Binder>
        struct indirectly_test_preparable;

        template<class S, class E, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_preparable_v = false;
        template<class S, class E>
        SQLITE_ORM_INLINE_VAR constexpr bool is_preparable_v<
            S,
            E,
            polyfill::void_t<indirectly_test_preparable<decltype(std::declval<S>().prepare(std::declval<E>()))>>> =
            true;

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

            storage_t(const storage_t&) = default;

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

                context_t context{this->db_objects};
                statement_serializer<Table, void> serializer;
                std::string sql = serializer.serialize(table, context, tableName);
                perform_void_exec(db, std::move(sql));
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
                this->drop_table_internal(db, table.name, false);
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

                this->drop_table_internal(db, table.name, false);

                this->rename_table(db, backupTableName, table.name);
            }

            template<class O>
            void assert_mapped_type() const {
                static_assert(tuple_has_type<db_objects_type, O, object_type_t>::value,
                              "type is not mapped to storage");
            }

            template<class O>
            void assert_updatable_type() const {
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
                using Table = storage_pick_table_t<O, db_objects_type>;
                using elements_type = elements_type_t<Table>;
                using col_index_sequence = filter_tuple_sequence_t<elements_type, is_column>;
                using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
                using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;
                constexpr size_t dedicatedPrimaryKeyColumnsCount =
                    nested_tuple_size_for_t<columns_tuple_t, elements_type, pk_index_sequence>::value;

                constexpr size_t primaryKeyColumnsCount =
                    dedicatedPrimaryKeyColumnsCount + pkcol_index_sequence::size();
                constexpr ptrdiff_t nonPrimaryKeysColumnsCount = col_index_sequence::size() - primaryKeyColumnsCount;
                static_assert(primaryKeyColumnsCount > 0, "A table without primary keys cannot be updated");
                static_assert(
                    nonPrimaryKeysColumnsCount > 0,
                    "A table with only primary keys cannot be updated. You need at least 1 non-primary key column");
#endif
            }

            template<class O,
                     class Table = storage_pick_table_t<O, db_objects_type>,
                     std::enable_if_t<Table::is_without_rowid::value, bool> = true>
            void assert_insertable_type() const {}

            template<class O,
                     class Table = storage_pick_table_t<O, db_objects_type>,
                     std::enable_if_t<!Table::is_without_rowid::value, bool> = true>
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
            template<class T, class O = mapped_type_proxy_t<T>, class... Args>
            mapped_view<O, self, Args...> iterate(Args&&... args) {
                this->assert_mapped_type<O>();

                auto con = this->get_connection();
                return {*this, std::move(con), std::forward<Args>(args)...};
            }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_refers_to_table auto mapped, class... Args>
            auto iterate(Args&&... args) {
                return this->iterate<decltype(mapped)>(std::forward<Args>(args)...);
            }
#endif

#if defined(SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED) && defined(SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED)
            template<class Select>
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
                requires(is_select_v<Select>)
#endif
            result_set_view<Select, db_objects_type> iterate(Select expression) {
                expression.highest_level = true;
                auto con = this->get_connection();
                return {this->db_objects, std::move(con), std::move(expression)};
            }

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            template<class... CTEs, class E>
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
                requires(is_select_v<E>)
#endif
            result_set_view<with_t<E, CTEs...>, db_objects_type> iterate(with_t<E, CTEs...> expression) {
                auto con = this->get_connection();
                return {this->db_objects, std::move(con), std::move(expression)};
            }
#endif
#endif

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table, class... Args>
            void remove_all(Args&&... args) {
                return this->remove_all<auto_decay_table_ref_t<table>>(std::forward<Args>(args)...);
            }
#endif

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table, class... Ids>
            void remove(Ids... ids) {
                return this->remove<auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
            }
#endif

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
             *  T is an explicitly specified object mapped to a storage or a table alias.
             *  R is an explicit return type. This type must have `push_back(O &&)` function. Defaults to `std::vector<O>`
             *  @return All objects of type O stored in database at the moment in `R`.
             *  @example: storage.get_all<User, std::list<User>>(); - SELECT * FROM users
             *  @example: storage.get_all<User, std::list<User>>(where(like(&User::name, "N%")), order_by(&User::id)); - SELECT * FROM users WHERE name LIKE 'N%' ORDER BY id
            */
            template<class T, class R = std::vector<mapped_type_proxy_t<T>>, class... Args>
            R get_all(Args&&... args) {
                this->assert_mapped_type<mapped_type_proxy_t<T>>();
                auto statement = this->prepare(sqlite_orm::get_all<T, R>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            /**
             *  SELECT * routine.
             *  `mapped` is an explicitly specified table reference or table alias of an object to be extracted.
             *  `R` is the container return type, which must have a `R::push_back(O&&)` method, and defaults to `std::vector<O>`
             *  @return All objects stored in database.
             *  @example: storage.get_all<sqlite_schema, std::list<sqlite_master>>(); - SELECT sqlite_schema.* FROM sqlite_master AS sqlite_schema
            */
            template<orm_refers_to_table auto mapped,
                     class R = std::vector<mapped_type_proxy_t<decltype(mapped)>>,
                     class... Args>
            R get_all(Args&&... args) {
                this->assert_mapped_type<mapped_type_proxy_t<decltype(mapped)>>();
                auto statement = this->prepare(sqlite_orm::get_all<mapped, R>(std::forward<Args>(args)...));
                return this->execute(statement);
            }
#endif

            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  R is a container type. std::vector<std::unique_ptr<O>> is default
             *  @return All objects of type O as std::unique_ptr<O> stored in database at the moment.
             *  @example: storage.get_all_pointer<User, std::list<std::unique_ptr<User>>>(); - SELECT * FROM users
             *  @example: storage.get_all_pointer<User, std::list<std::unique_ptr<User>>>(where(length(&User::name) > 6)); - SELECT * FROM users WHERE LENGTH(name)  > 6
            */
            template<class O, class R = std::vector<std::unique_ptr<O>>, class... Args>
            auto get_all_pointer(Args&&... args) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all_pointer<O, R>(std::forward<Args>(args)...));
                return this->execute(statement);
            }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table,
                     class R = std::vector<std::unique_ptr<auto_decay_table_ref_t<table>>>,
                     class... Args>
            auto get_all_pointer(Args&&... args) {
                return this->get_all_pointer<auto_decay_table_ref_t<table>>(std::forward<Args>(args)...);
            }
#endif

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            /**
             *  SELECT * routine.
             *  O is an object type to be extracted. Must be specified explicitly.
             *  R is a container type. std::vector<std::optional<O>> is default
             *  @return All objects of type O as std::optional<O> stored in database at the moment.
             *  @example: storage.get_all_optional<User, std::list<std::optional<O>>>(); - SELECT * FROM users
             *  @example: storage.get_all_optional<User, std::list<std::optional<O>>>(where(length(&User::name) > 6)); - SELECT * FROM users WHERE LENGTH(name)  > 6
            */
            template<class O, class R = std::vector<std::optional<O>>, class... Args>
            auto get_all_optional(Args&&... conditions) {
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::get_all_optional<O, R>(std::forward<Args>(conditions)...));
                return this->execute(statement);
            }
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table,
                     class R = std::vector<std::optional<auto_decay_table_ref_t<table>>>,
                     class... Args>
            auto get_all_optional(Args&&... conditions) {
                return this->get_all_optional<auto_decay_table_ref_t<table>>(std::forward<Args>(conditions)...);
            }
#endif

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table, class... Ids>
            auto get(Ids... ids) {
                return this->get<auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
            }
#endif

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table, class... Ids>
            auto get_pointer(Ids... ids) {
                return this->get_pointer<auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
            }
#endif

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_table_reference auto table, class... Ids>
            auto get_optional(Ids... ids) {
                return this->get_optional<auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
            }
#endif

            /**
             *  SELECT COUNT(*) https://www.sqlite.org/lang_aggfunc.html#count
             *  @return Number of O object in table.
             */
            template<class O, class... Args>
            int count(Args&&... args) {
                using R = mapped_type_proxy_t<O>;
                this->assert_mapped_type<R>();
                auto rows = this->select(sqlite_orm::count<R>(), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return rows.front();
                } else {
                    return 0;
                }
            }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<orm_refers_to_table auto mapped, class... Args>
            int count(Args&&... args) {
                return this->count<auto_decay_table_ref_t<mapped>>(std::forward<Args>(args)...);
            }
#endif

            /**
             *  SELECT COUNT(X) https://www.sqlite.org/lang_aggfunc.html#count
             *  @param m member pointer to class mapped to the storage.
             *  @return count of `m` values from database.
             */
            template<class F,
                     class... Args,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            int count(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                auto rows = this->select(sqlite_orm::count(std::move(field)), std::forward<Args>(args)...);
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
            template<class F,
                     class... Args,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            double avg(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                auto rows = this->select(sqlite_orm::avg(std::move(field)), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    return rows.front();
                } else {
                    return 0;
                }
            }

            template<class F,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::string group_concat(F field) {
                return this->group_concat_internal(std::move(field), {});
            }

            /**
             *  GROUP_CONCAT(X) query.  https://www.sqlite.org/lang_aggfunc.html#groupconcat
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return group_concat query result.
             */
            template<class F,
                     class... Args,
                     class Tuple = std::tuple<Args...>,
                     std::enable_if_t<std::tuple_size<Tuple>::value >= 1, bool> = true,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::string group_concat(F field, Args&&... args) {
                return this->group_concat_internal(std::move(field), {}, std::forward<Args>(args)...);
            }

            /**
             *  GROUP_CONCAT(X, Y) query.   https://www.sqlite.org/lang_aggfunc.html#groupconcat
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return group_concat query result.
             */
            template<class F,
                     class... Args,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::string group_concat(F field, std::string y, Args&&... args) {
                return this->group_concat_internal(std::move(field),
                                                   std::make_unique<std::string>(std::move(y)),
                                                   std::forward<Args>(args)...);
            }

            template<class F,
                     class... Args,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::string group_concat(F field, const char* y, Args&&... args) {
                std::unique_ptr<std::string> str;
                if(y) {
                    str = std::make_unique<std::string>(y);
                } else {
                    str = std::make_unique<std::string>();
                }
                return this->group_concat_internal(std::move(field), std::move(str), std::forward<Args>(args)...);
            }

            /**
             *  MAX(x) query.
             *  @param m is a class member pointer (the same you passed into make_column).
             *  @return std::unique_ptr with max value or null if sqlite engine returned null.
             */
            template<class F,
                     class... Args,
                     class R = column_result_of_t<db_objects_type, F>,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::unique_ptr<R> max(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                auto rows = this->select(sqlite_orm::max(std::move(field)), std::forward<Args>(args)...);
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
            template<class F,
                     class... Args,
                     class R = column_result_of_t<db_objects_type, F>,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::unique_ptr<R> min(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                auto rows = this->select(sqlite_orm::min(std::move(field)), std::forward<Args>(args)...);
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
            template<class F,
                     class... Args,
                     class R = column_result_of_t<db_objects_type, F>,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            std::unique_ptr<R> sum(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                std::vector<std::unique_ptr<double>> rows =
                    this->select(sqlite_orm::sum(std::move(field)), std::forward<Args>(args)...);
                if(!rows.empty()) {
                    if(rows.front()) {
                        return std::make_unique<R>(std::move(*rows.front()));
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
            template<class F,
                     class... Args,
                     std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value,
                                      bool> = true>
            double total(F field, Args&&... args) {
                this->assert_mapped_type<table_type_of_t<F>>();
                auto rows = this->select(sqlite_orm::total(std::move(field)), std::forward<Args>(args)...);
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
            template<class T, class... Args>
            auto select(T m, Args... args) {
                static_assert(!is_compound_operator_v<T> || sizeof...(Args) == 0,
                              "Cannot use args with a compound operator");
                auto statement = this->prepare(sqlite_orm::select(std::move(m), std::forward<Args>(args)...));
                return this->execute(statement);
            }

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            /**
             *  Using a CTE, select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             */
            template<class CTE, class E>
            auto with(CTE cte, E expression) {
                auto statement = this->prepare(sqlite_orm::with(std::move(cte), std::move(expression)));
                return this->execute(statement);
            }

            /**
             *  Using a CTE, select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             */
            template<class... CTEs, class E>
            auto with(common_table_expressions<CTEs...> cte, E expression) {
                auto statement = this->prepare(sqlite_orm::with(std::move(cte), std::move(expression)));
                return this->execute(statement);
            }

            /**
             *  Using a CTE, select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             */
            template<class CTE, class E>
            auto with_recursive(CTE cte, E expression) {
                auto statement = this->prepare(sqlite_orm::with_recursive(std::move(cte), std::move(expression)));
                return this->execute(statement);
            }

            /**
             *  Using a CTE, select a single column into std::vector<T> or multiple columns into std::vector<std::tuple<...>>.
             */
            template<class... CTEs, class E>
            auto with_recursive(common_table_expressions<CTEs...> cte, E expression) {
                auto statement = this->prepare(sqlite_orm::with_recursive(std::move(cte), std::move(expression)));
                return this->execute(statement);
            }
#endif

            template<class T, satisfies<is_prepared_statement, T> = true>
            std::string dump(const T& preparedStatement, bool parametrized = true) const {
                return this->dump_highest_level(preparedStatement.expression, parametrized);
            }

            template<class E,
                     class Ex = polyfill::remove_cvref_t<E>,
                     std::enable_if_t<!is_prepared_statement<Ex>::value && !is_mapped<db_objects_type, Ex>::value,
                                      bool> = true>
            std::string dump(E&& expression, bool parametrized = false) const {
                static_assert(is_preparable_v<self, Ex>, "Expression must be a high-level statement");

                decltype(auto) e2 = static_if<is_select<Ex>::value>(
                    [](auto expression) -> auto {
                        expression.highest_level = true;
                        return expression;
                    },
                    [](const auto& expression) -> decltype(auto) {
                        return (expression);
                    })(std::forward<E>(expression));
                return this->dump_highest_level(e2, parametrized);
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
                using O = std::decay_t<decltype(polyfill::invoke(project, *from))>;
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
            SQLITE_ORM_INSERT_RETURN_TYPE insert(const O& o, columns_t<Cols...> cols) {
                static_assert(cols.count > 0, "Use insert or replace with 1 argument instead");
                this->assert_mapped_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(std::ref(o), std::move(cols)));
                return SQLITE_ORM_INSERT_RETURN_TYPE(this->execute(statement));
            }

            /**
             *  Insert routine. Inserts object with all non primary key fields in passed object. Id of passed
             *  object doesn't matter.
             *  @return id of just created object.
             */
            template<class O>
            SQLITE_ORM_INSERT_RETURN_TYPE insert(const O& o) {
                this->assert_mapped_type<O>();
                this->assert_insertable_type<O>();
                auto statement = this->prepare(sqlite_orm::insert(std::ref(o)));
                return SQLITE_ORM_INSERT_RETURN_TYPE(this->execute(statement));
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
            template<class M>
            sync_schema_result schema_status(const virtual_table_t<M>&, sqlite3*, bool, bool*) {
                return sync_schema_result::already_in_sync;
            }

            template<class T, class... S>
            sync_schema_result schema_status(const trigger_t<T, S...>&, sqlite3*, bool, bool*) {
                return sync_schema_result::already_in_sync;
            }

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

            template<class M>
            sync_schema_result sync_table(const virtual_table_t<M>& virtualTable, sqlite3* db, bool) {
                auto res = sync_schema_result::already_in_sync;
                using context_t = serializer_context<db_objects_type>;
                context_t context{this->db_objects};
                auto query = serialize(virtualTable, context);
                perform_void_exec(db, query);
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
                auto query = serialize(trigger, context);
                perform_void_exec(db, query);
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

            template<class ColResult, class S>
            auto execute_select(const S& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                using R = decltype(make_row_extractor<ColResult>(this->db_objects).extract(nullptr, 0));
                std::vector<R> res;
                perform_steps(
                    stmt,
                    [rowExtractor = make_row_extractor<ColResult>(this->db_objects), &res](sqlite3_stmt* stmt) {
                        res.push_back(rowExtractor.extract(stmt, 0));
                    });
                res.shrink_to_fit();
                return res;
            }

            template<class E>
            std::string dump_highest_level(E&& expression, bool parametrized) const {
                const auto& exprDBOs = db_objects_for_expression(this->db_objects, expression);
                using context_t = serializer_context<polyfill::remove_cvref_t<decltype(exprDBOs)>>;
                context_t context{exprDBOs};
                context.replace_bindable_with_question = parametrized;
                // just like prepare_impl()
                context.skip_table_name = false;
                return serialize(expression, context);
            }

            template<typename S>
            prepared_statement_t<S> prepare_impl(S statement) {
                const auto& exprDBOs = db_objects_for_expression(this->db_objects, statement);
                using context_t = serializer_context<polyfill::remove_cvref_t<decltype(exprDBOs)>>;
                context_t context{exprDBOs};
                context.skip_table_name = false;
                context.replace_bindable_with_question = true;

                auto con = this->get_connection();
                std::string sql = serialize(statement, context);
                sqlite3_stmt* stmt = prepare_stmt(con.get(), std::move(sql));
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

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            template<class... CTEs,
                     class E,
                     std::enable_if_t<polyfill::disjunction_v<is_select<E>, is_insert_raw<E>>, bool> = true>
            prepared_statement_t<with_t<E, CTEs...>> prepare(with_t<E, CTEs...> sel) {
                return this->prepare_impl<with_t<E, CTEs...>>(std::move(sel));
            }
#endif

            template<class T, class... Args>
            prepared_statement_t<select_t<T, Args...>> prepare(select_t<T, Args...> statement) {
                statement.highest_level = true;
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Args>
            prepared_statement_t<get_all_t<T, Args...>> prepare(get_all_t<T, Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Args>
            prepared_statement_t<get_all_pointer_t<T, Args...>> prepare(get_all_pointer_t<T, Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class... Args>
            prepared_statement_t<replace_raw_t<Args...>> prepare(replace_raw_t<Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class... Args>
            prepared_statement_t<insert_raw_t<Args...>> prepare(insert_raw_t<Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class R, class... Args>
            prepared_statement_t<get_all_optional_t<T, R, Args...>>
            prepare(get_all_optional_t<T, R, Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class S, class... Wargs>
            prepared_statement_t<update_all_t<S, Wargs...>> prepare(update_all_t<S, Wargs...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Args>
            prepared_statement_t<remove_all_t<T, Args...>> prepare(remove_all_t<T, Args...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Ids>
            prepared_statement_t<get_t<T, Ids...>> prepare(get_t<T, Ids...> statement) {
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Ids>
            prepared_statement_t<get_pointer_t<T, Ids...>> prepare(get_pointer_t<T, Ids...> statement) {
                return this->prepare_impl(std::move(statement));
            }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            template<class T, class... Ids>
            prepared_statement_t<get_optional_t<T, Ids...>> prepare(get_optional_t<T, Ids...> statement) {
                return this->prepare_impl(std::move(statement));
            }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

            template<class T>
            prepared_statement_t<update_t<T>> prepare(update_t<T> statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                this->assert_updatable_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Ids>
            prepared_statement_t<remove_t<T, Ids...>> prepare(remove_t<T, Ids...> statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class T>
            prepared_statement_t<insert_t<T>> prepare(insert_t<T> statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                this->assert_insertable_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class T>
            prepared_statement_t<replace_t<T>> prepare(replace_t<T> statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class E, satisfies<is_insert_range, E> = true>
            prepared_statement_t<E> prepare(E statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                this->assert_insertable_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class E, satisfies<is_replace_range, E> = true>
            prepared_statement_t<E> prepare(E statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class T, class... Cols>
            prepared_statement_t<insert_explicit<T, Cols...>> prepare(insert_explicit<T, Cols...> statement) {
                using object_type = expression_object_type_t<decltype(statement)>;
                this->assert_mapped_type<object_type>();
                return this->prepare_impl(std::move(statement));
            }

            template<class... Args>
            void execute(const prepared_statement_t<replace_raw_t<Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression, conditional_binder{stmt});
                perform_step(stmt);
            }

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            template<class... CTEs, class E, satisfies<is_insert_raw, E> = true>
            void execute(const prepared_statement_t<with_t<E, CTEs...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression, conditional_binder{stmt});
                perform_step(stmt);
            }
#endif

            template<class... Args>
            void execute(const prepared_statement_t<insert_raw_t<Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                iterate_ast(statement.expression, conditional_binder{stmt});
                perform_step(stmt);
            }

            template<class T, class... Cols>
            int64 execute(const prepared_statement_t<insert_explicit<T, Cols...>>& statement) {
                using object_type = statement_object_type_t<decltype(statement)>;

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
                     std::enable_if_t<polyfill::disjunction<is_replace<T>, is_replace_range<T>>::value, bool> = true>
            void execute(const prepared_statement_t<T>& statement) {
                using object_type = statement_object_type_t<decltype(statement)>;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                auto processObject = [&table = this->get_table<object_type>(),
                                      bindValue = field_value_binder{stmt}](auto& object) mutable {
                    table.template for_each_column_excluding<is_generated_always>(
                        call_as_template_base<column_field>([&bindValue, &object](auto& column) {
                            bindValue(polyfill::invoke(column.member_pointer, object));
                        }));
                };

                static_if<is_replace_range<T>::value>(
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

            template<class T,
                     std::enable_if_t<polyfill::disjunction<is_insert<T>, is_insert_range<T>>::value, bool> = true>
            int64 execute(const prepared_statement_t<T>& statement) {
                using object_type = statement_object_type_t<decltype(statement)>;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                auto processObject = [&table = this->get_table<object_type>(),
                                      bindValue = field_value_binder{stmt}](auto& object) mutable {
                    using is_without_rowid = typename std::decay_t<decltype(table)>::is_without_rowid;
                    table.template for_each_column_excluding<
                        mpl::conjunction<mpl::not_<mpl::always<is_without_rowid>>,
                                         mpl::disjunction_fn<is_primary_key, is_generated_always>>>(
                        call_as_template_base<column_field>([&table, &bindValue, &object](auto& column) {
                            if(!exists_in_composite_primary_key(table, column)) {
                                bindValue(polyfill::invoke(column.member_pointer, object));
                            }
                        }));
                };

                static_if<is_insert_range<T>::value>(
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
                using object_type = statement_object_type_t<decltype(statement)>;

                sqlite3_stmt* stmt = reset_stmt(statement.stmt);
                auto& table = this->get_table<object_type>();

                field_value_binder bindValue{stmt};
                auto& object = get_object(statement.expression);
                table.template for_each_column_excluding<mpl::disjunction_fn<is_primary_key, is_generated_always>>(
                    call_as_template_base<column_field>([&table, &bindValue, &object](auto& column) {
                        if(!exists_in_composite_primary_key(table, column)) {
                            bindValue(polyfill::invoke(column.member_pointer, object));
                        }
                    }));
                table.for_each_column([&table, &bindValue, &object](auto& column) {
                    if(column.template is<is_primary_key>() || exists_in_composite_primary_key(table, column)) {
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

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            template<class... CTEs, class T, class... Args>
            auto execute(const prepared_statement_t<with_t<select_t<T, Args...>, CTEs...>>& statement) {
                using ExprDBOs = decltype(db_objects_for_expression(this->db_objects, statement.expression));
                // note: it is enough to only use the 'expression DBOs' at compile-time to determine the column results;
                // because we cannot select objects/structs from a CTE, passing the permanently defined DBOs are enough.
                using ColResult = column_result_of_t<ExprDBOs, T>;
                return this->execute_select<ColResult>(statement);
            }
#endif

            template<class T, class... Args>
            auto execute(const prepared_statement_t<select_t<T, Args...>>& statement) {
                using ColResult = column_result_of_t<db_objects_type, T>;
                return this->execute_select<ColResult>(statement);
            }

            template<class T, class R, class... Args, class O = mapped_type_proxy_t<T>>
            R execute(const prepared_statement_t<get_all_t<T, R, Args...>>& statement) {
                sqlite3_stmt* stmt = reset_stmt(statement.stmt);

                iterate_ast(statement.expression, conditional_binder{stmt});

                R res;
                perform_steps(stmt, [&table = this->get_table<O>(), &res](sqlite3_stmt* stmt) {
                    O obj;
                    object_from_column_builder<O> builder{obj, stmt};
                    table.for_each_column(builder);
                    res.push_back(std::move(obj));
                });
#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
                if constexpr(polyfill::is_specialization_of_v<R, std::vector>) {
                    res.shrink_to_fit();
                }
#endif
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
#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
                if constexpr(polyfill::is_specialization_of_v<R, std::vector>) {
                    res.shrink_to_fit();
                }
#endif
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
#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
                if constexpr(polyfill::is_specialization_of_v<R, std::vector>) {
                    res.shrink_to_fit();
                }
#endif
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
