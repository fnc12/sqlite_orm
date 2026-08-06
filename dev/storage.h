#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr/shared_ptr, std::make_unique
#include <system_error>  //  std::system_error
#include <string>  //  std::string
#include <type_traits>  //  std::remove_reference, std::remove_cvref, std::decay
#include <functional>  //   std::identity
#include <sstream>  //  std::stringstream
#include <ostream>  //  std::flush
#include <map>  //  std::map
#include <vector>  //  std::vector
#include <tuple>  //  std::tuple_size, std::tuple, std::make_tuple, std::tie
#include <utility>  //  std::forward, std::pair
#include <algorithm>  //  std::for_each, std::ranges::for_each
#ifdef SQLITE_ORM_CPP23_GENERATOR_SUPPORTED
#include <generator>
#endif
#endif
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cxx_functional_polyfill.h"
#include "functional/gsl.h"
#include "functional/mpl.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "alias.h"
#include "error_code.h"
#include "column_constraints.h"
#include "schema/traits/constraining_traits.h"
#include "field_printer.h"
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
#include "mapped_view.h"
#include "result_set_view.h"
#include "ast_iterator.h"
#include "storage_base.h"
#include "prepared_statement.h"
#include "expression_object_type.h"
#include "statement_serializer.h"
#include "serializer_context.h"
#include "object_from_column_builder.h"
#include "row_extractor.h"
#include "schema/table.h"
#include "schema/view.h"
#include "schema/virtual_table.h"
#include "schema/column.h"
#include "schema/index.h"
#include "schema/triggers.h"
#include "cte_storage.h"
#include "util.h"
#include "serializing_util.h"
#include "udf_existence_checker.h"

namespace sqlite_orm::internal {
    /*
     *  Implementation note: the technique of indirect expression testing is because
     *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
     *  It must also be a type that differs from those for `is_printable_v`, `is_bindable_v`.
     */
    template<class Binder>
    struct indirectly_test_preparable;

    template<class S, class E, class SFINAE = void>
    inline constexpr bool is_preparable_v = false;
    template<class S, class E>
    inline constexpr bool is_preparable_v<
        S,
        E,
        polyfill::void_t<indirectly_test_preparable<decltype(std::declval<S>().prepare(std::declval<E>()))>>> = true;

    template<class Opt, class OptionsTpl>
    decltype(auto) storage_opt_or_default([[maybe_unused]] OptionsTpl& options) {
        if constexpr (tuple_has_type<OptionsTpl, Opt>::value) {
            return std::move(std::get<Opt>(options));
        } else {
            return Opt{};
        }
    }

    /**
     *  Storage class itself. Create an instance to use it as an interfacto to sqlite db by calling `make_storage`
     *  function.
     */
    template<class... DBO>
    struct storage_t : storage_base {
        using self_type = storage_t;
        using db_objects_type = db_objects_tuple<DBO...>;

        /**
         *  @param filename database filename.
         *  @param dbObjects db_objects_tuple
         */
        template<class OptionsTpl>
        storage_t(std::string filename, db_objects_type dbObjects, OptionsTpl options) :
            storage_base{std::move(filename),
                         storage_opt_or_default<connection_control>(options),
                         storage_opt_or_default<on_open_spec>(options),
                         storage_opt_or_default<will_run_query_spec>(options),
                         storage_opt_or_default<did_run_query_spec>(options),
                         foreign_keys_count<db_objects_type>()},
            db_objects{std::move(dbObjects)} {
#ifdef SQLITE_ORM_WITH_VIEW
            this->validate_dbos();
#endif
        }

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
        friend const db_objects_type& obtain_db_objects(const self_type& storage) noexcept {
            return storage.db_objects;
        }

#ifdef SQLITE_ORM_WITH_VIEW
        void validate_dbos() const {
            // validate views: a view cannot select sub-objects, and column results must be convertible to view's object type
            iterate_tuple<db_objects_type>(views_index_sequence<db_objects_type>{}, [this](const auto* view) {
                using DrivingSelect = polyfill::remove_cvref_t<decltype(access_main_select(view->select))>;
                using ExprDBOs =
                    polyfill::remove_cvref_t<decltype(db_objects_for_expression(this->db_objects, view->select))>;
                using ColResult = column_result_of_t<ExprDBOs, DrivingSelect>;
                using elements_type = elements_type_t<std::remove_reference_t<decltype(*view)>>;
                using field_types = transform_tuple_t<filter_tuple_t<elements_type, is_column>, field_type_t>;

                static_assert(std::is_same<column_result_proxy_t<ColResult>, ColResult>::value,
                              "A view cannot select sub-objects");
                static_assert(std::is_convertible<tuplify_t<ColResult>, field_types>::value,
                              "Column results must be convertible to view's object type");
            });
        }
#endif

        template<class Table>
        void create_table(sqlite3* db, const std::string& tableName, const Table& table) {
            using context_t = serializer_context<db_objects_type>;

            context_t context{this->db_objects};
            statement_serializer<Table, void> serializer;
            const std::string sql = serializer.serialize(table, context, tableName);
            this->executor.perform_void_exec(db, sql.c_str());
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
            std::string sql;
            {
                std::stringstream ss;
                ss << "ALTER TABLE " << streaming_identifier(tableName) << " DROP COLUMN "
                   << streaming_identifier(columnName) << std::flush;
                sql = ss.str();
            }
            this->executor.perform_void_exec(db, sql.c_str());
        }
#endif

        template<class Table>
        void drop_create_with_loss(sqlite3* db, const Table& table) {
            // eliminated all transaction handling
            this->drop_dbo_internal(db, "TABLE", table.name, false);
            this->create_table(db, table.name, table);
        }

        template<class Table>
        void backup_table(sqlite3* db, const Table& table, const std::vector<const table_xinfo*>& columnsToIgnore) {

            //  here we copy source table to another with a name with '_backup' suffix, but in case table with such
            //  a name already exists we append suffix 1, then 2, etc until we find a free name..
            auto backupTableName = table.name + "_backup";
            if (this->object_exists(db, "table", backupTableName)) {
                int suffix = 1;
                do {
                    std::stringstream ss;
                    ss << suffix << std::flush;
                    auto anotherBackupTableName = backupTableName + ss.str();
                    if (!this->object_exists(db, "table", anotherBackupTableName)) {
                        backupTableName = std::move(anotherBackupTableName);
                        break;
                    }
                    ++suffix;
                } while (true);
            }
            this->create_table(db, backupTableName, table);

            this->copy_table(db, table.name, backupTableName, table, columnsToIgnore);

            this->drop_dbo_internal(db, "TABLE", table.name, false);

            this->rename_table_internal(db, backupTableName, table.name);
        }

        template<class O>
        void assert_mapped_type() const {
            static_assert(tuple_has_type<db_objects_type, O, object_type_t>::value, "type is not mapped to storage");
        }

        template<class O>
        void assert_primary_key_type() const {
            using table_type = storage_pick_table_t<O, db_objects_type>;
            using elements_type = elements_type_t<table_type>;
            using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
            using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;

            static_assert(pk_index_sequence::size() + pkcol_index_sequence::size() == 1,
                          "The table must have a primary key");
        }

        template<class O>
        void assert_updatable_type() const {
            using table_type = storage_pick_table_t<O, db_objects_type>;
            using elements_type = elements_type_t<table_type>;
            using column_index_sequence = col_index_sequence_of<elements_type>;
            using pk_index_sequence = filter_tuple_sequence_t<elements_type, is_primary_key>;
            using pkcol_index_sequence = col_index_sequence_with<elements_type, is_primary_key>;
            constexpr size_t nTablePrimaryKeyColumns =
                nested_tuple_size_for_t<columns_tuple_t, elements_type, pk_index_sequence>::value;

            constexpr size_t nPrimaryKeyColumns = nTablePrimaryKeyColumns + pkcol_index_sequence::size();
            constexpr ptrdiff_t nNonPrimaryKeysColumns = column_index_sequence::size() - nPrimaryKeyColumns;
            static_assert(nPrimaryKeyColumns > 0, "A table without primary keys cannot be updated");
            static_assert(
                nNonPrimaryKeysColumns > 0,
                "A table with only primary keys cannot be updated. You need at least 1 non-primary key column");
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
            if constexpr (pkcol_index_sequence::size()) {
                constexpr auto pkcol_idx = index_sequence_value_at<0>(pkcol_index_sequence{});
                using pkcol_type = std::tuple_element_t<pkcol_idx, elements_type>;
                static_assert(
                    mpl::invoke_t<check_if<is_pkcol_implicitly_insertable>, pkcol_type>::value,
                    "While SQLite allows primary keys of any type, sqlite_orm restricts an ordinary 'insert' into "
                    "tables with single-column primary keys to those with an implicitly insertable column because "
                    "it is the 'rowid' alias or has a default value."
                    "Instead, please use `replace(object)` or `insert(object, columns(...))`.");
            }
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
        /*  
         *  Iterate over objects of a type mapped as a table, lazily fetched from a result set.
         *  
         *  The returned C++ view models a C++ input range and is also a 'borrowed range',
         *  meaning that iterators obtained from it are not tied to the lifetime of the view instance.
         */
        template<class T, class O = mapped_type_proxy_t<T>, class... Args>
        mapped_view<O, self_type, Args...> iterate(Args&&... args) {
            this->assert_mapped_type<O>();

            auto conRef = this->get_connection();
            return {*this, std::move(conRef), std::forward<Args>(args)...};
        }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /*  
         *  Iterate over objects of a type mapped as a table, lazily fetched from a result set.
         *  
         *  The returned C++ view models a C++ input range and is also a 'borrowed range',
         *  meaning that iterators obtained from it are not tied to the lifetime of the view instance.
         */
        template<orm_refers_to_table auto mapped, class... Args>
        auto iterate(Args&&... args) {
            return this->iterate<decltype(mapped)>(std::forward<Args>(args)...);
        }
#endif

        /*  
         *  Iterate over a result set of a select statement or a select statement involving a common table expression.
         *  
         *  The returned C++ view models a C++ input range and is also a 'borrowed range',
         *  meaning that iterators obtained from it are not tied to the lifetime of the view instance.
         */
        template<class Select>
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            requires (is_select_expression_v<Select>)
#endif
        result_set_view<Select, db_objects_type> iterate(Select expression) {
#ifndef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
            static_assert(is_select_expression_v<Select>,
                          "SQL expression must be a select expression or a with-clause with a select expression");
#endif
            if constexpr (is_select_v<Select>) {
                expression.highest_level = true;
            }
            auto conRef = this->get_connection();
            return {this->db_objects, std::move(conRef), std::move(expression)};
        }

#ifdef SQLITE_ORM_CPP23_GENERATOR_SUPPORTED
        /*  
         *  Iterate over objects of a type mapped as a table, lazily fetched from a result set in a coroutine.
         */
        template<class T, class O = mapped_type_proxy_t<T>, class... Args>
        std::generator<O> yield(Args&&... args) {
            this->assert_mapped_type<O>();
            // implementation note: instead of using `this->iterate<O>()` we iterate over a select statement,
            // because a `mapped_view` has a legacy input iterator that returns a reference to an object.
            // For a generator we want to yield objects by value that can be moved from.
            for (O obj: this->iterate(sqlite_orm::select(struct_<O>(asterisk<T>(true)), std::forward<Args>(args)...))) {
                co_yield obj;
            }
        }

        /*  
         *  Iterate over objects of a type mapped as a table, lazily fetched from a result set in a coroutine.
         */
        template<orm_refers_to_table auto mapped, class O = mapped_type_proxy_t<decltype(mapped)>, class... Args>
        std::generator<O> yield(Args&&... args) {
            this->assert_mapped_type<O>();
            // implementation note: instead of using `this->iterate<O>()` we iterate over a select statement,
            // because a `mapped_view` has a legacy input iterator that returns a reference to an object.
            // For a generator we want to yield objects by value that can be moved from.
            for (O obj: this->iterate(
                     sqlite_orm::select(struct_<O>(asterisk<decltype(mapped)>(true)), std::forward<Args>(args)...))) {
                co_yield obj;
            }
        }

        /*  
         *  Iterate over a result set of a select statement or a select statement involving a common table expression in a coroutine.
         */
        template<class Select>
            requires (is_select_expression_v<Select>)
        auto yield(Select expression) -> std::generator<decltype(*this->iterate(std::move(expression)).begin())> {
            for (auto row: this->iterate(std::move(expression))) {
                co_yield row;
            }
        }
#endif

        /**
         *  Delete from routine.
         *  O is an object's type. Must be specified explicitly.
         *  @param args optional conditions: `where`, `join` etc
         *  @example: storage.remove_all<User>(); - DELETE FROM users
         *  @example: storage.remove_all<User>(where(in(&User::id, {5, 6, 7}))); - DELETE FROM users WHERE id IN (5, 6, 7)
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
            static_assert((internal::is_bindable_v<Ids> && ...), "Only primary key values are accepted as Ids");
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
         *  compiler from first parameter.
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
            static_assert(internal::is_set<S>::value, "first argument in update_all can be either set or dynamic_set");
            auto statement = this->prepare(sqlite_orm::update_all(std::move(set), std::forward<Wargs>(wh)...));
            this->execute(statement);
        }

      protected:
        template<class F, class O, class... Args>
        std::string group_concat_internal(F O::* m, std::unique_ptr<std::string> y, Args&&... args) {
            this->assert_mapped_type<O>();
            std::vector<std::string> rows;
            if (y) {
                rows = this->select(sqlite_orm::group_concat(m, std::move(*y)), std::forward<Args>(args)...);
            } else {
                rows = this->select(sqlite_orm::group_concat(m), std::forward<Args>(args)...);
            }
            if (!rows.empty()) {
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
         *  id. throws std::system_error with orm_error_category in case of db error. O is an object type to be
         *  extracted. Must be specified explicitly.
         *  @return Object of type O where id is equal parameter passed or throws
         *  `std::system_error{orm_error_code::not_found}` if there is no object with such id.
         */
        template<class O, class... Ids>
        O get(Ids... ids) {
            static_assert((internal::is_bindable_v<Ids> && ...), "Only primary key values are accepted as Ids");
            this->assert_mapped_type<O>();
            this->assert_primary_key_type<O>();
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
         *  with null value. throws std::system_error in case of db error.
         */
        template<class O, class... Ids>
        std::unique_ptr<O> get_pointer(Ids... ids) {
            static_assert((internal::is_bindable_v<Ids> && ...), "Only primary key values are accepted as Ids");
            this->assert_mapped_type<O>();
            this->assert_primary_key_type<O>();
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
         *  A previous version of get_pointer() that returns a shared_ptr
         *  instead of a unique_ptr. New code should prefer get_pointer()
         *  unless the data needs to be shared.
         *  
         *  @note
         *  Most scenarios don't need shared ownership of data, so we should prefer
         *  unique_ptr when possible. It's more efficient, doesn't require atomic
         *  ops for a reference count (which can cause major slowdowns on
         *  weakly-ordered platforms like ARM), and can be easily promoted to a
         *  shared_ptr, exactly like we're doing here.
         *  (Conversely, you _can't_ go from shared back to unique.)
         */
        template<class O, class... Ids>
        std::shared_ptr<O> get_no_throw(Ids... ids) {
            return std::shared_ptr<O>(this->get_pointer<O>(std::forward<Ids>(ids)...));
        }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        /**
         *  The same as `get` function but doesn't throw an exception if noting found but
         *  returns an empty std::optional. throws std::system_error in case of db error.
         */
        template<class O, class... Ids>
        std::optional<O> get_optional(Ids... ids) {
            static_assert((internal::is_bindable_v<Ids> && ...), "Only primary key values are accepted as Ids");
            this->assert_mapped_type<O>();
            this->assert_primary_key_type<O>();
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
            if (!rows.empty()) {
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        int count(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            auto rows = this->select(sqlite_orm::count(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        double avg(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            auto rows = this->select(sqlite_orm::avg(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
                return rows.front();
            } else {
                return 0;
            }
        }

        template<class F,
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        std::string group_concat(F field, std::string y, Args&&... args) {
            return this->group_concat_internal(std::move(field),
                                               std::make_unique<std::string>(std::move(y)),
                                               std::forward<Args>(args)...);
        }

        template<class F,
                 class... Args,
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        std::string group_concat(F field, orm_gsl::czstring y, Args&&... args) {
            std::unique_ptr<std::string> str;
            if (y) {
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        std::unique_ptr<R> max(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            auto rows = this->select(sqlite_orm::max(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        std::unique_ptr<R> min(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            auto rows = this->select(sqlite_orm::min(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
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
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        std::unique_ptr<R> sum(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            std::vector<std::unique_ptr<double>> rows =
                this->select(sqlite_orm::sum(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
                if (rows.front()) {
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
         *  https://www.sqlite.org/lang_aggfunc.html)
         */
        template<class F,
                 class... Args,
                 std::enable_if_t<polyfill::disjunction<std::is_member_pointer<F>, is_column_pointer<F>>::value, bool> =
                     true>
        double total(F field, Args&&... args) {
            this->assert_mapped_type<table_type_of_t<F>>();
            auto rows = this->select(sqlite_orm::total(std::move(field)), std::forward<Args>(args)...);
            if (!rows.empty()) {
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

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
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

        template<
            class E,
            class Ex = polyfill::remove_cvref_t<E>,
            std::enable_if_t<!is_prepared_statement<Ex>::value && !is_mapped<db_objects_type, Ex>::value, bool> = true>
        std::string dump(E&& expression, bool parametrized = false) const {
            static_assert(is_preparable_v<self_type, Ex>, "Expression must be a high-level statement");

            if constexpr (is_select_v<Ex>) {
                auto e2 = std::forward<E>(expression);
                e2.highest_level = true;
                return this->dump_highest_level(e2, parametrized);
            } else {
                return this->dump_highest_level(expression, parametrized);
            }
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
                using field_type = field_type_t<std::remove_reference_t<decltype(column)>>;
                static constexpr std::array<orm_gsl::czstring, 2> sep = {", ", ""};

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
            if (from == to) {
                return;
            }

            auto statement =
                this->prepare(sqlite_orm::replace_range(std::move(from), std::move(to), std::move(project)));
            this->execute(statement);
        }

        template<class O, class It, class Projection = polyfill::identity>
        void replace_range(It from, It to, Projection project = {}) {
            this->assert_mapped_type<O>();
            if (from == to) {
                return;
            }

            auto statement =
                this->prepare(sqlite_orm::replace_range<O>(std::move(from), std::move(to), std::move(project)));
            this->execute(statement);
        }

        /**
         *  Insert routine with explicitly specified columns.
         *  
         *  @return The ID of the last inserted record for a rowid table, otherwise a meaningless value.
         *          Attention: `sqlite3_last_insert_rowid()` is used to retrieve the last inserted ID, therefore the ID is only useful in single-threaded contexts.
         *          Attention: While SQLite returns a 64-bit integer as rowid, this function returns an `int` that most likely has less precision.
         *                     If you need the full 64-bit rowid value, use `storage_t<>::execute()` instead, or call `storage_t<>::last_insert_rowid()` after inserting.
         */
        template<class O, class... Cols>
        int insert(const O& o, columns_t<Cols...> cols) {
            static_assert(cols.count > 0, "Use insert or replace with 1 argument instead");
            this->assert_mapped_type<O>();
            auto statement = this->prepare(sqlite_orm::insert(std::ref(o), std::move(cols)));
            return int(this->execute(statement));
        }

        /**
         *  Ordinary insert routine.
         *  
         *  - For objects mapped to a rowid table with a single primary key:
         *      Inserts a record with all fields of a mapped object except the primary key column.
         *      The primary key column must be implicitly insertable.
         *      The 'ID' of the specified object is irrelevant as it is implicitly inserted.
         *  - For objects mapped to a rowid table with a composite primary key or no primary key:
         *    Inserts a record with all fields of a mapped object except primary key columns having a default value.
         *  - For objects mapped to a table without rowid:
         *    Inserts a record with all fields of a mapped object except primary key columns having a default value.
         *  
         *  @return The ID of the last inserted record for a rowid table, otherwise a meaningless value.
         *          Attention: `sqlite3_last_insert_rowid()` is used to retrieve the last inserted ID, therefore the ID is only useful in single-threaded contexts.
         *          Attention: While SQLite returns a 64-bit integer as rowid, this function returns an `int` that most likely has less precision.
         *                     If you need the full 64-bit rowid value, use `storage_t<>::execute()` instead, or call `storage_t<>::last_insert_rowid()` after inserting.
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
            if (from == to) {
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
            if (from == to) {
                return;
            }
            auto statement =
                this->prepare(sqlite_orm::insert_range<O>(std::move(from), std::move(to), std::move(project)));
            this->execute(statement);
        }

        /**
         *  Change table name inside storage's schema info. This function does not
         *  affect database
         */
        template<class O>
        void rename_table(std::string name) {
            this->assert_mapped_type<O>();
            auto& table = this->get_table<O>();
            table.name = std::move(name);
        }

        using storage_base::rename_table;

        /**
         *  Get table's name stored in storage's schema info. This function does not call
         *  any SQLite queries
         */
        template<class O>
        const std::string& tablename() const {
            this->assert_mapped_type<O>();
            auto& table = this->get_table<O>();
            return table.name;
        }

        template<class F, class O>
        [[deprecated("Use the more accurately named function `find_column_name()`")]] const std::string*
        column_name(F O::* memberPointer) const {
            return internal::find_column_name(this->db_objects, memberPointer);
        }

        template<class F, class O>
        const std::string* find_column_name(F O::* memberPointer) const {
            return internal::find_column_name(this->db_objects, memberPointer);
        }

      protected:
        template<class Table, satisfies<is_virtual_table, Table> = true>
        sync_schema_result schema_status(const Table&, sqlite3*, bool, bool*) {
            return sync_schema_result::already_in_sync;
        }

        template<class T, class... S>
        sync_schema_result schema_status(const trigger_t<T, S...>& trigger, sqlite3* db, bool, bool*) {
            auto dbTriggerSql = this->retrieve_object_sql(db, "trigger", trigger.name);
            if (dbTriggerSql.empty()) {
                return sync_schema_result::new_table_created;
            }

            const serializer_context<db_objects_type> context{this->db_objects};
            auto storageSql = serialize(trigger, context);

            if (dbTriggerSql == storageSql) {
                return sync_schema_result::already_in_sync;
            }
            return sync_schema_result::dropped_and_recreated;
        }

        template<class... Cols>
        sync_schema_result schema_status(const index_t<Cols...>&, sqlite3*, bool, bool*) {
            return sync_schema_result::already_in_sync;
        }

#ifdef SQLITE_ORM_WITH_VIEW
        template<class View, satisfies<is_view, View> = true>
        sync_schema_result schema_status(const View& queryView, sqlite3* db, bool, bool*) {
            auto dbViewSql = this->retrieve_object_sql(db, "view", queryView.name);
            if (dbViewSql.empty()) {
                return sync_schema_result::new_table_created;
            }

            const auto& exprDBOs = db_objects_for_expression(this->db_objects, queryView.select);

            using context_t = serializer_context<std::remove_cvref_t<decltype(exprDBOs)>>;
            const context_t context{exprDBOs};
            auto storageSql = serialize(queryView, context);

            if (dbViewSql == storageSql) {
                return sync_schema_result::already_in_sync;
            }
            return sync_schema_result::dropped_and_recreated;
        }
#endif

        template<class Table, satisfies<is_base_table, Table> = true>
        sync_schema_result schema_status(const Table& table, sqlite3* db, bool preserve, bool* attempt_to_preserve) {
            if (attempt_to_preserve) {
                *attempt_to_preserve = true;
            }

            auto dbTableInfo = this->pragma.table_xinfo(table.name);
            auto res = sync_schema_result::already_in_sync;
            bool canPreserveData = true;

            //  first let's see if table with such name exists..
            auto gottaCreateTable = !this->object_exists(db, "table", table.name);
            if (!gottaCreateTable) {

                //  get table info provided in `make_table` call..
                auto storageTableInfo = table.get_table_info();

                //  this vector will contain pointers to columns that gotta be added..
                std::vector<const table_xinfo*> columnsToAdd;

                if (calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo)) {
                    gottaCreateTable = true;
                }

                if (!gottaCreateTable) {  //  if all storage columns are equal to actual db columns but there are
                    //  excess columns at the db..
                    if (!dbTableInfo.empty()) {
                        // extra table columns than storage columns
                        if (!preserve) {
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
                if (gottaCreateTable) {
                    // check if any new columns prevent data preservation
                    for (const table_xinfo* colInfo: columnsToAdd) {
                        if (!table.find_column_generated_storage_type(colInfo->name)) {
                            if (colInfo->notnull && colInfo->dflt_value.empty()) {
                                canPreserveData = false;
                                if (attempt_to_preserve) {
                                    *attempt_to_preserve = false;
                                };
                                break;
                            }
                        }
                    }
                    res = canPreserveData ? sync_schema_result::dropped_and_recreated
                                          : sync_schema_result::dropped_and_recreated_with_data_loss;
                } else {
                    if (!columnsToAdd.empty()) {
                        // extra storage columns than table columns
                        for (const table_xinfo* colInfo: columnsToAdd) {
                            const basic_generated_always::storage_type* generatedStorageType =
                                table.find_column_generated_storage_type(colInfo->name);
                            if (generatedStorageType) {
                                if (*generatedStorageType == basic_generated_always::storage_type::stored) {
                                    gottaCreateTable = true;
                                    break;
                                }
                                //  fallback cause VIRTUAL can be added
                            } else {
                                if (colInfo->notnull && colInfo->dflt_value.empty()) {
                                    gottaCreateTable = true;
                                    canPreserveData = false;
                                    // no matter if preserve is true or false, there is no way to preserve data, so we wont try!
                                    if (attempt_to_preserve) {
                                        *attempt_to_preserve = false;
                                    };
                                    break;
                                }
                            }
                        }
                        if (!gottaCreateTable) {
                            if (res == sync_schema_result::old_columns_removed) {
                                res = sync_schema_result::new_columns_added_and_old_columns_removed;
                            } else {
                                res = sync_schema_result::new_columns_added;
                            }
                        } else {
                            res = canPreserveData ? sync_schema_result::dropped_and_recreated
                                                  : sync_schema_result::dropped_and_recreated_with_data_loss;
                        }
                    } else {
                        if (res != sync_schema_result::old_columns_removed) {
                            res = sync_schema_result::already_in_sync;
                        }
                    }
                }
            } else {
                res = sync_schema_result::new_table_created;
            }
            return res;
        }

        template<class Table, satisfies<is_virtual_table, Table> = true>
        sync_schema_result sync_dbo(const Table& virtualTable, sqlite3* db, bool) {
            // eponymous virtual table instances with the same name as their module exist already
            if constexpr (Table::module_traits_type::is_eponymous::value) {
                if (virtualTable.name == Table::module_type::name()) {
                    return sync_schema_result::already_in_sync;
                }
            }

            const auto res = sync_schema_result::already_in_sync;
            const serializer_context<db_objects_type> context{this->db_objects};
            const auto sql = serialize(virtualTable, context);
            this->executor.perform_void_exec(db, sql.c_str());
            return res;
        }

        template<class... Cols>
        sync_schema_result sync_dbo(const index_t<Cols...>& index, sqlite3* db, bool) {
            const auto res = sync_schema_result::already_in_sync;
            const serializer_context<db_objects_type> context{this->db_objects};
            const auto sql = serialize(index, context);
            this->executor.perform_void_exec(db, sql.c_str());
            return res;
        }

        template<class... Cols>
        sync_schema_result sync_dbo(const trigger_t<Cols...>& trigger, sqlite3* db, bool preserve) {
            auto res = this->schema_status(trigger, db, preserve, nullptr);
            if (res != sync_schema_result::already_in_sync) {
                if (res == sync_schema_result::dropped_and_recreated) {
                    this->drop_dbo_internal(db, "TRIGGER", trigger.name, true);
                }
                const serializer_context<db_objects_type> context{this->db_objects};
                const auto sql = serialize(trigger, context);
                this->executor.perform_void_exec(db, sql.c_str());
            }
            return res;
        }

#ifdef SQLITE_ORM_WITH_VIEW
        template<class View, satisfies<is_view, View> = true>
        sync_schema_result sync_dbo(const View& queryView, sqlite3* db, bool preserve) {
            auto res = this->schema_status(queryView, db, preserve, nullptr);
            if (res != sync_schema_result::already_in_sync) {
                if (res == sync_schema_result::dropped_and_recreated) {
                    this->drop_dbo_internal(db, "VIEW", queryView.name, true);
                }

                const auto& exprDBOs = db_objects_for_expression(this->db_objects, queryView.select);

                using context_t = serializer_context<polyfill::remove_cvref_t<decltype(exprDBOs)>>;
                const context_t context{exprDBOs};
                const auto sql = serialize(queryView, context);
                this->executor.perform_void_exec(db, sql.c_str());
            }
            return res;
        }
#endif

        template<class Table, satisfies<is_base_table, Table> = true>
        sync_schema_result sync_dbo(const Table& table, sqlite3* db, bool preserve);

        template<class Table, satisfies<is_base_table, Table> = true>
        sync_schema_result sync_regular_base_table(const Table& table, sqlite3* db, bool preserve);

        template<class C>
        void add_column(sqlite3* db, const std::string& tableName, const C& column) const {
            using context_t = serializer_context<db_objects_type>;

            context_t context{this->db_objects};
            std::string sql;
            {
                std::stringstream ss;
                ss << "ALTER TABLE " << streaming_identifier(tableName) << " ADD COLUMN " << serialize(column, context)
                   << std::flush;
                sql = ss.str();
            }
            this->executor.perform_void_exec(db, sql.c_str());
        }

        template<class ColResult, class S>
        auto execute_select(const S& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            iterate_ast(statement.expression, conditional_binder{stmt});

            using R = decltype(make_row_extractor<ColResult>(this->db_objects).extract(nullptr, 0));

            std::vector<R> res;
            this->executor.perform_steps(
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
            context.omit_table_name = false;
            return serialize(expression, context);
        }

        template<typename S>
        prepared_statement_t<S> prepare_impl(S statement) {
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            // check the existence of application-defined functions used in the statement
            iterate_ast(
                statement,
                udf_existence_checker{this->scalarFunctions, this->aggregateFunctions, this->collatingFunctions});
#endif

            const auto& exprDBOs = db_objects_for_expression(this->db_objects, statement);

            using context_t = serializer_context<polyfill::remove_cvref_t<decltype(exprDBOs)>>;
            context_t context{exprDBOs};
            context.omit_table_name = false;
            context.replace_bindable_with_question = true;

            const std::string sql = serialize(statement, context);
            auto conRef = this->get_connection();
            sqlite3_stmt* stmt = prepare_stmt(conRef.get(), sql);
            return prepared_statement_t<S>{std::forward<S>(statement), stmt, std::move(conRef)};
        }

      public:
        /**
         *  This is a cute function used to replace migration up/down functionality.
         *  It performs check storage schema with actual db schema and:
         *  - if there are excess tables exist in db they are ignored (not dropped)
         *  - every table from storage is compared with it's db analog and
         *      - if table doesn't exist it is being created
         *      - if table exists its colums are being compared with table_info from db and
         *          - if there are columns in db that do not exist in storage (excess) table will be dropped and recreated
         *          - if there are columns in storage that do not exist in db they will be added using `ALTER TABLE ... ADD COLUMN ...' command
         *          - if there is any column existing in both db and storage but differs by any of
         *  properties/constraints (pk, notnull, dflt_value) table will be dropped and recreated. Be aware that
         *  `sync_schema` doesn't guarantee that data will not be dropped. It guarantees only that it will make db
         *  schema the same as you specified in `make_storage` function call. A good point is that if you have no db
         *  file at all it will be created and all tables also will be created with exact tables and columns you
         *  specified in `make_storage`, `make_table` and `make_column` calls. The best practice is to call this
         *  function right after storage creation.
         *  @param preserve affects function's behaviour in case it is needed to remove a column. If it is `false`
         *  so table will be dropped if there is column to remove if SQLite version is < 3.35.0 and remove column if SQLite version >= 3.35.0,
         *  if `true` -  table is being copied into another table, dropped and copied table is renamed with source table name.
         *  Warning: sync_schema doesn't check foreign keys cause it is unable to do so in sqlite3. If you know how to get foreign key info please
         *  submit an issue https://github.com/fnc12/sqlite_orm/issues
         *  @return std::map with std::string key equal table name and `sync_schema_result` as value.
         *  `sync_schema_result` is a enum value that stores table state after syncing a schema. `sync_schema_result`
         *  can be printed out on std::ostream with `operator<<`.
         */
        std::map<std::string, sync_schema_result> sync_schema(bool preserve = false) {
            auto conRef = this->get_connection();
            std::map<std::string, sync_schema_result> result;
            iterate_tuple<true>(this->db_objects, [this, db = conRef.get(), preserve, &result](auto& schemaObject) {
                sync_schema_result status = this->sync_dbo(schemaObject, db, preserve);
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
            auto conRef = this->get_connection();
            std::map<std::string, sync_schema_result> result;
            iterate_tuple<true>(this->db_objects, [this, db = conRef.get(), preserve, &result](auto& schemaObject) {
                sync_schema_result status = this->schema_status(schemaObject, db, preserve, nullptr);
                result.emplace(schemaObject.name, status);
            });
            return result;
        }

        template<class DML, std::enable_if_t<is_raw_dml_expression_v<DML>, bool> = true>
        prepared_statement_t<DML> prepare(DML statement) {
            return this->prepare_impl(std::move(statement));
        }

        template<class Select, satisfies<is_select_expression, Select> = true>
        prepared_statement_t<Select> prepare(Select statement) {
            if constexpr (is_select_v<Select>) {
                statement.highest_level = true;
            }
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

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class R, class... Args>
        prepared_statement_t<get_all_optional_t<T, R, Args...>> prepare(get_all_optional_t<T, R, Args...> statement) {
            return this->prepare_impl(std::move(statement));
        }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

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
            if (statement.range.first == statement.range.second) {
                throw std::system_error{orm_error_code::empty_range};
            }
            return this->prepare_impl(std::move(statement));
        }

        template<class E, satisfies<is_replace_range, E> = true>
        prepared_statement_t<E> prepare(E statement) {
            using object_type = expression_object_type_t<decltype(statement)>;
            this->assert_mapped_type<object_type>();
            if (statement.range.first == statement.range.second) {
                throw std::system_error{orm_error_code::empty_range};
            }
            return this->prepare_impl(std::move(statement));
        }

        template<class T, class... Cols>
        prepared_statement_t<insert_explicit<T, Cols...>> prepare(insert_explicit<T, Cols...> statement) {
            using object_type = expression_object_type_t<decltype(statement)>;
            this->assert_mapped_type<object_type>();
            return this->prepare_impl(std::move(statement));
        }

        template<class DML, std::enable_if_t<is_raw_dml_expression_v<DML>, bool> = true>
        void execute(const prepared_statement_t<DML>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);
            iterate_ast(statement.expression, conditional_binder{stmt});
            this->executor.perform_single_step(stmt);
        }

        /** 
         *  @return The ID of the last inserted record for a table with rowid, otherwise a meaningless value.
         *          Attention: `sqlite3_last_insert_rowid()` is used to retrieve the last inserted ID, therefore the ID is only useful in single-threaded contexts.
         */
        template<class T, class... Cols>
        int64 execute(const prepared_statement_t<insert_explicit<T, Cols...>>& statement) {
            using object_type = statement_object_type_t<decltype(statement)>;

            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            tuple_value_binder{stmt}(
                statement.expression.columns.columns,
                [&table = this->get_table<object_type>(), &object = statement.expression.obj](auto& memberPointer) {
                    return table.object_field_value(object, memberPointer);
                });

            this->executor.perform_single_step(stmt);
            return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt));
        }

        template<class T,
                 std::enable_if_t<polyfill::disjunction<is_replace<T>, is_replace_range<T>>::value, bool> = true>
        void execute(const prepared_statement_t<T>& statement) {
            using object_type = statement_object_type_t<decltype(statement)>;

            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            auto processObject = [&table = this->get_table<object_type>(),
                                  bindValue = field_value_binder{stmt}](const object_type& object) mutable {
                table.template for_each_column_excluding<is_generated_always>(
                    call_as_template_base<column_field>([&bindValue, &object](auto& column) {
                        bindValue(polyfill::invoke(column.member_pointer, object));
                    }));
            };

            if constexpr (is_replace_range_v<T>) {
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
                std::ranges::for_each(statement.expression.range.first,
                                      statement.expression.range.second,
                                      std::ref(processObject),
                                      std::ref(statement.expression.transformer));
#else
                auto& transformer = statement.expression.transformer;
                std::for_each(statement.expression.range.first,
                              statement.expression.range.second,
                              [&processObject, &transformer](auto&& item) {
                                  using item_type = decltype(item);
                                  const object_type& object =
                                      polyfill::invoke(transformer, std::forward<item_type>(item));
                                  processObject(object);
                              });
#endif
            } else {
                const object_type& object = get_object(statement.expression);
                processObject(object);
            };

            this->executor.perform_single_step(stmt);
        }

        /** 
         *  @return The ID of the last inserted record for a table with rowid, otherwise a meaningless value.
         *          Attention: `sqlite3_last_insert_rowid()` is used to retrieve the last inserted ID, therefore the ID is only useful in single-threaded contexts.
         */
        template<class T, std::enable_if_t<polyfill::disjunction<is_insert<T>, is_insert_range<T>>::value, bool> = true>
        int64 execute(const prepared_statement_t<T>& statement) {
            using object_type = statement_object_type_t<decltype(statement)>;

            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            auto processObject = [&table = this->get_table<object_type>(),
                                  bindValue = field_value_binder{stmt}](const object_type& object) mutable {
                using table_type = polyfill::remove_cvref_t<decltype(table)>;
                using is_pkcolumn_q = mpl::conjunction<mpl::not_<mpl::always<typename table_type::is_without_rowid>>,
                                                       mpl::quote_fn<is_primary_key>>;
                using is_generated_always_q = mpl::quote_fn<is_generated_always>;

                table.template for_each_column_excluding<mpl::disjunction<is_pkcolumn_q, is_generated_always_q>>(
                    [&table, &bindValue, &object](auto& column) {
                        if (!table_type::is_without_rowid::value &&
                            (is_single_table_primary_key(table, column) ||
                             (column.template is_template<default_t>() && table_primary_key_contains(table, column)))) {
                            return;
                        } else if (table_type::is_without_rowid::value && (column.template is_template<default_t>() &&
                                                                           table_primary_key_contains(table, column))) {
                            return;
                        }
                        bindValue(polyfill::invoke(column.member_pointer, object));
                    });
            };

            if constexpr (is_insert_range_v<T>) {
#ifdef SQLITE_ORM_CPP20_RANGES_SUPPORTED
                std::ranges::for_each(statement.expression.range.first,
                                      statement.expression.range.second,
                                      std::ref(processObject),
                                      std::ref(statement.expression.transformer));
#else
                auto& transformer = statement.expression.transformer;
                std::for_each(statement.expression.range.first,
                              statement.expression.range.second,
                              [&processObject, &transformer](auto&& item) {
                                  using item_type = decltype(item);
                                  const object_type& object =
                                      polyfill::invoke(transformer, std::forward<item_type>(item));
                                  processObject(object);
                              });
#endif
            } else {
                const object_type& object = get_object(statement.expression);
                processObject(object);
            }

            this->executor.perform_single_step(stmt);
            return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt));
        }

        template<class T, class... Ids>
        void execute(const prepared_statement_t<remove_t<T, Ids...>>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);
            iterate_ast(statement.expression.ids, conditional_binder{stmt});
            this->executor.perform_single_step(stmt);
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
                    if (table_primary_key_contains(table, column)) {
                        return;
                    }
                    bindValue(polyfill::invoke(column.member_pointer, object));
                }));
            table.for_each_column([&table, &bindValue, &object](auto& column) {
                if (!column.template is<is_primary_key>() && !table_primary_key_contains(table, column)) {
                    return;
                }
                bindValue(polyfill::invoke(column.member_pointer, object));
            });

            this->executor.perform_single_step(stmt);
        }

        template<class T, class... Ids>
        std::unique_ptr<T> execute(const prepared_statement_t<get_pointer_t<T, Ids...>>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            iterate_ast(statement.expression.ids, conditional_binder{stmt});

            std::unique_ptr<T> res;
            this->executor.perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
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
            this->executor.perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
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
            this->executor.perform_step(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                object_from_column_builder<T> builder{res.emplace(), stmt};
                table.for_each_column(builder);
            });
            if (!res.has_value()) {
                throw std::system_error{orm_error_code::not_found};
            }
            return std::move(res).value();
#else
            auto& table = this->get_table<T>();

            std::string sql;
            if (this->executor.will_run_query || this->executor.did_run_query) {
                sql = statement.sql();
            }
            if (this->executor.will_run_query) {
                this->executor.will_run_query(sql);
            }

            switch (SQLITE_ORM_SWITCH_MAYBE_UNUSED int rc = sqlite3_step(stmt)) {
                case SQLITE_ROW:
                    break;
                case SQLITE_DONE: {
                    throw std::system_error{orm_error_code::not_found};
                }
                default: {
                    throw_translated_sqlite_error(stmt);
                }
            }

            T res;
            object_from_column_builder<T> builder{res, stmt};
            table.for_each_column(builder);

            if (this->executor.did_run_query) {
                this->executor.did_run_query(sql);
            }

            return res;
#endif
        }

        template<class Select, satisfies<is_select_expression, Select> = true>
        auto execute(const prepared_statement_t<Select>& statement) {
            using ExprDBOs =
                polyfill::remove_cvref_t<decltype(db_objects_for_expression(this->db_objects, statement.expression))>;
            // note: it is enough to only use the 'expression DBOs' at compile-time to determine the column results;
            // because we cannot select objects/structs from a CTE, passing the permanently defined DBOs are enough.
            using ColResult = column_result_of_t<ExprDBOs, main_select_t<Select>>;
            return this->execute_select<ColResult>(statement);
        }

        template<class T, class R, class... Args, class O = mapped_type_proxy_t<T>>
        R execute(const prepared_statement_t<get_all_t<T, R, Args...>>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            iterate_ast(statement.expression, conditional_binder{stmt});

            R res;
            this->executor.perform_steps(stmt, [&table = this->get_table<O>(), &res](sqlite3_stmt* stmt) {
                O obj;
                object_from_column_builder<O> builder{obj, stmt};
                table.for_each_column(builder);
                res.push_back(std::move(obj));
            });

            if constexpr (polyfill::is_specialization_of_v<R, std::vector>) {
                res.shrink_to_fit();
            }

            return res;
        }

        template<class T, class R, class... Args>
        R execute(const prepared_statement_t<get_all_pointer_t<T, R, Args...>>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            iterate_ast(statement.expression, conditional_binder{stmt});

            R res;
            this->executor.perform_steps(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                auto obj = std::make_unique<T>();
                object_from_column_builder<T> builder{*obj, stmt};
                table.for_each_column(builder);
                res.push_back(std::move(obj));
            });

            if constexpr (polyfill::is_specialization_of_v<R, std::vector>) {
                res.shrink_to_fit();
            }

            return res;
        }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class R, class... Args>
        R execute(const prepared_statement_t<get_all_optional_t<T, R, Args...>>& statement) {
            sqlite3_stmt* stmt = reset_stmt(statement.stmt);

            iterate_ast(statement.expression, conditional_binder{stmt});

            R res;
            this->executor.perform_steps(stmt, [&table = this->get_table<T>(), &res](sqlite3_stmt* stmt) {
                auto obj = std::make_optional<T>();
                object_from_column_builder<T> builder{*obj, stmt};
                table.for_each_column(builder);
                res.push_back(std::move(obj));
            });

            if constexpr (polyfill::is_specialization_of_v<R, std::vector>) {
                res.shrink_to_fit();
            }

            return res;
        }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
    };  // struct storage_t

    template<class Elements>
    using dbo_index_sequence = filter_tuple_sequence_t<Elements, check_if_lacks<storage_opt_tag_t>::template fn>;

    template<class Elements>
    using opt_index_sequence = filter_tuple_sequence_t<Elements, check_if_names<storage_opt_tag_t>::template fn>;

    template<class... DBO, class OptionsTpl>
    storage_t<DBO...> make_storage(std::string filename, std::tuple<DBO...> dbObjects, OptionsTpl options) {
        return {std::move(filename), std::move(dbObjects), std::move(options)};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /*
     *  Factory function for a storage instance, from a database file, a set of database object definitions
     *  and option storage options like connection control options and an 'on open' callback.
     *  
     *  E.g.
     *  auto storage = make_storage("", connection_control{.open_forever = true}, on_open([](sqlite3* db) {}));
     */
    template<class... Spec>
    auto make_storage(std::string filename, Spec... specifications) {
        using namespace ::sqlite_orm::internal;

        std::tuple specTuple{std::forward<Spec>(specifications)...};
        return internal::make_storage(
            std::move(filename),
            create_from_tuple<std::tuple>(std::move(specTuple), dbo_index_sequence<decltype(specTuple)>{}),
            create_from_tuple<std::tuple>(std::move(specTuple), opt_index_sequence<decltype(specTuple)>{}));
    }

    /**
     *  sqlite3_threadsafe() interface.
     */
    inline int threadsafe() {
        return sqlite3_threadsafe();
    }
}
