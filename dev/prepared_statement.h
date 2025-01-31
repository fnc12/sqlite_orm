#pragma once

#include <sqlite3.h>
#ifndef _IMPORT_STD_MODULE
#include <memory>  //  std::unique_ptr
#include <iterator>  //  std::iterator_traits
#include <string>  //  std::string
#include <type_traits>  //  std::integral_constant, std::declval
#include <utility>  //  std::move, std::forward, std::pair
#include <tuple>  //  std::tuple
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cxx_functional_polyfill.h"
#include "tuple_helper/tuple_traits.h"
#include "connection_holder.h"
#include "select_constraints.h"
#include "values.h"
#include "table_reference.h"
#include "mapped_type_proxy.h"
#include "ast/upsert_clause.h"
#include "ast/set.h"

namespace sqlite_orm {

    namespace internal {

        struct prepared_statement_base {
            sqlite3_stmt* stmt = nullptr;
            connection_ref con;

            ~prepared_statement_base() {
                sqlite3_finalize(this->stmt);
            }

            std::string sql() const {
                // note: sqlite3 internally checks for null before calling
                // sqlite3_normalized_sql() or sqlite3_expanded_sql(), so check here, too, even if superfluous
                if (const char* sql = sqlite3_sql(this->stmt)) {
                    return sql;
                } else {
                    return {};
                }
            }

#if SQLITE_VERSION_NUMBER >= 3014000
            std::string expanded_sql() const {
                // note: must check return value due to SQLITE_OMIT_TRACE
                using char_ptr = std::unique_ptr<char, std::integral_constant<decltype(&sqlite3_free), sqlite3_free>>;
                if (char_ptr sql{sqlite3_expanded_sql(this->stmt)}) {
                    return sql.get();
                } else {
                    return {};
                }
            }
#endif
#if SQLITE_VERSION_NUMBER >= 3026000 and defined(SQLITE_ENABLE_NORMALIZE)
            std::string normalized_sql() const {
                if (const char* sql = sqlite3_normalized_sql(this->stmt)) {
                    return sql;
                } else {
                    return {};
                }
            }
#endif

#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            std::string_view column_name(int index) const {
                return sqlite3_column_name(stmt, index);
            }
#endif
        };

        template<class T>
        struct prepared_statement_t : prepared_statement_base {
            using expression_type = T;

            expression_type expression;

            prepared_statement_t(T expression_, sqlite3_stmt* stmt_, connection_ref con_) :
                prepared_statement_base{stmt_, std::move(con_)}, expression(std::move(expression_)) {}

            prepared_statement_t(prepared_statement_t&& prepared_stmt) :
                prepared_statement_base{prepared_stmt.stmt, std::move(prepared_stmt.con)},
                expression(std::move(prepared_stmt.expression)) {
                prepared_stmt.stmt = nullptr;
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_prepared_statement_v =
            polyfill::is_specialization_of<T, prepared_statement_t>::value;

        template<class T>
        struct is_prepared_statement : polyfill::bool_constant<is_prepared_statement_v<T>> {};

        /**
         *  T - type of object to obtain from a database
         */
        template<class T, class R, class... Args>
        struct get_all_t {
            using type = T;
            using return_type = R;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };

        template<class T, class R, class... Args>
        struct get_all_pointer_t {
            using type = T;
            using return_type = R;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class R, class... Args>
        struct get_all_optional_t {
            using type = T;
            using return_type = R;

            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class S, class... Wargs>
        struct update_all_t {
            using set_type = S;
            using conditions_type = std::tuple<Wargs...>;

            static_assert(is_set<S>::value, "update_all_t must have set or dynamic set as the first argument");

            set_type set;
            conditions_type conditions;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_update_all_v = polyfill::is_specialization_of<T, update_all_t>::value;

        template<class T>
        using is_update_all = polyfill::bool_constant<is_update_all_v<T>>;

        template<class T, class... Args>
        struct remove_all_t {
            using type = T;
            using conditions_type = std::tuple<Args...>;

            conditions_type conditions;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_remove_all_v = polyfill::is_specialization_of<T, remove_all_t>::value;

        template<class T>
        using is_remove_all = polyfill::bool_constant<is_remove_all_v<T>>;

        template<class T, class... Ids>
        struct get_t {
            using type = T;
            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };

        template<class T, class... Ids>
        struct get_pointer_t {
            using type = T;
            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Ids>
        struct get_optional_t {
            using type = T;
            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class T>
        struct update_t {
            using type = T;

            type object;
        };

        template<class T, class... Ids>
        struct remove_t {
            using type = T;
            using ids_type = std::tuple<Ids...>;

            ids_type ids;
        };

        template<class T>
        struct insert_t {
            using type = T;

            type object;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_insert_v = polyfill::is_specialization_of<T, insert_t>::value;

        template<class T>
        struct is_insert : polyfill::bool_constant<is_insert_v<T>> {};

        template<class T, class... Cols>
        struct insert_explicit {
            using type = T;
            using columns_type = columns_t<Cols...>;

            type obj;
            columns_type columns;
        };

        template<class T>
        struct replace_t {
            using type = T;

            type object;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_replace_v = polyfill::is_specialization_of<T, replace_t>::value;

        template<class T>
        struct is_replace : polyfill::bool_constant<is_replace_v<T>> {};

        template<class It, class Projection, class O>
        struct insert_range_t {
            using iterator_type = It;
            using transformer_type = Projection;
            using object_type = O;

            std::pair<iterator_type, iterator_type> range;
            transformer_type transformer;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_insert_range_v =
            polyfill::is_specialization_of<T, insert_range_t>::value;

        template<class T>
        struct is_insert_range : polyfill::bool_constant<is_insert_range_v<T>> {};

        template<class It, class Projection, class O>
        struct replace_range_t {
            using iterator_type = It;
            using transformer_type = Projection;
            using object_type = O;

            std::pair<iterator_type, iterator_type> range;
            transformer_type transformer;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_replace_range_v =
            polyfill::is_specialization_of<T, replace_range_t>::value;

        template<class T>
        struct is_replace_range : polyfill::bool_constant<is_replace_range_v<T>> {};

        template<class... Args>
        struct insert_raw_t {
            using args_tuple = std::tuple<Args...>;

            args_tuple args;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_insert_raw_v = polyfill::is_specialization_of<T, insert_raw_t>::value;

        template<class T>
        struct is_insert_raw : polyfill::bool_constant<is_insert_raw_v<T>> {};

        template<class... Args>
        struct replace_raw_t {
            using args_tuple = std::tuple<Args...>;

            args_tuple args;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_replace_raw_v = polyfill::is_specialization_of<T, replace_raw_t>::value;

        template<class T>
        struct is_replace_raw : polyfill::bool_constant<is_replace_raw_v<T>> {};

        struct default_values_t {};

        template<class T>
        using is_default_values = std::is_same<T, default_values_t>;

        enum class conflict_action {
            abort,
            fail,
            ignore,
            replace,
            rollback,
        };

        struct insert_constraint {
            conflict_action action = conflict_action::abort;
        };

        template<class T>
        using is_insert_constraint = std::is_same<T, insert_constraint>;
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    inline internal::insert_constraint or_rollback() {
        return {internal::conflict_action::rollback};
    }

    inline internal::insert_constraint or_replace() {
        return {internal::conflict_action::replace};
    }

    inline internal::insert_constraint or_ignore() {
        return {internal::conflict_action::ignore};
    }

    inline internal::insert_constraint or_fail() {
        return {internal::conflict_action::fail};
    }

    inline internal::insert_constraint or_abort() {
        return {internal::conflict_action::abort};
    }

    /**
     *  Use this function to add `DEFAULT VALUES` modifier to raw `INSERT`.
     *
     *  @example
     *  ```
     *  storage.insert(into<Singer>(), default_values());
     *  ```
     */
    inline internal::default_values_t default_values() {
        return {};
    }

    /**
     *  Raw insert statement creation routine. Use this if `insert` with object does not fit you. This insert is designed to be able
     *  to call any type of `INSERT` query with no limitations.
     *  @example
     *  ```sql
     *  INSERT INTO users (id, name) VALUES(5, 'Little Mix')
     *  ```
     *  will be
     *  ```c++
     *  auto statement = storage.prepare(insert(into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix"))));
     *  storage.execute(statement));
     *  ```
     *  One more example:
     *  ```sql
     *  INSERT INTO singers (name) VALUES ('Sofia Reyes')('Kungs')
     *  ```
     *  will be
     *  ```c++
     *  auto statement = storage.prepare(insert(into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs"))));
     *  storage.execute(statement));
     *  ```
     *  One can use `default_values` to add `DEFAULT VALUES` modifier:
     *  ```sql
     *  INSERT INTO users DEFAULT VALUES
     *  ```
     *  will be
     *  ```c++
     *  auto statement = storage.prepare(insert(into<Singer>(), default_values()));
     *  storage.execute(statement));
     *  ```
     *  Also one can use `INSERT OR ABORT`/`INSERT OR FAIL`/`INSERT OR IGNORE`/`INSERT OR REPLACE`/`INSERT ROLLBACK`:
     *  ```c++
     *  auto statement = storage.prepare(insert(or_ignore(), into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs"))));
     *  auto statement2 = storage.prepare(insert(or_rollback(), into<Singer>(), default_values()));
     *  auto statement3 = storage.prepare(insert(or_abort(), into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix"))));
     *  ```
     */
    template<class... Args>
    internal::insert_raw_t<Args...> insert(Args... args) {
        using args_tuple = std::tuple<Args...>;
        using internal::count_tuple;
        using internal::is_columns;
        using internal::is_insert_constraint;
        using internal::is_into;
        using internal::is_select;
        using internal::is_upsert_clause;
        using internal::is_values;

        constexpr int orArgsCount = count_tuple<args_tuple, is_insert_constraint>::value;
        static_assert(orArgsCount < 2, "Raw insert must have only one OR... argument");

        constexpr int intoArgsCount = count_tuple<args_tuple, is_into>::value;
        static_assert(intoArgsCount != 0, "Raw insert must have into<T> argument");
        static_assert(intoArgsCount < 2, "Raw insert must have only one into<T> argument");

        constexpr int columnsArgsCount = count_tuple<args_tuple, is_columns>::value;
        static_assert(columnsArgsCount < 2, "Raw insert must have only one columns(...) argument");

        constexpr int valuesArgsCount = count_tuple<args_tuple, is_values>::value;
        static_assert(valuesArgsCount < 2, "Raw insert must have only one values(...) argument");

        constexpr int defaultValuesCount = count_tuple<args_tuple, internal::is_default_values>::value;
        static_assert(defaultValuesCount < 2, "Raw insert must have only one default_values() argument");

        constexpr int selectsArgsCount = count_tuple<args_tuple, is_select>::value;
        static_assert(selectsArgsCount < 2, "Raw insert must have only one select(...) argument");

        constexpr int upsertClausesCount = count_tuple<args_tuple, is_upsert_clause>::value;
        static_assert(upsertClausesCount <= 2, "Raw insert can contain 2 instances of upsert clause maximum");

        constexpr int argsCount = int(std::tuple_size<args_tuple>::value);
        static_assert(argsCount == intoArgsCount + columnsArgsCount + valuesArgsCount + defaultValuesCount +
                                       selectsArgsCount + orArgsCount + upsertClausesCount,
                      "Raw insert has invalid arguments");

        return {{std::forward<Args>(args)...}};
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
     *  auto statement = storage.prepare(replace(into<User>, columns(&User::id, &User::name), values(std::make_tuple(5, "Little Mix"))));
     *  storage.execute(statement));
     *  ```
     *  One more example:
     *  ```sql
     *  REPLACE INTO singers (name) VALUES ('Sofia Reyes')('Kungs')
     *  ```
     *  will be
     *  ```c++
     *  auto statement = storage.prepare(replace(into<Singer>(), columns(&Singer::name), values(std::make_tuple("Sofia Reyes"), std::make_tuple("Kungs"))));
     *  storage.execute(statement));
     *  ```
     *  One can use `default_values` to add `DEFAULT VALUES` modifier:
     *  ```sql
     *  REPLACE INTO users DEFAULT VALUES
     *  ```
     *  will be
     *  ```c++
     *  auto statement = storage.prepare(replace(into<Singer>(), default_values()));
     *  storage.execute(statement));
     *  ```
     */
    template<class... Args>
    internal::replace_raw_t<Args...> replace(Args... args) {
        using args_tuple = std::tuple<Args...>;
        using internal::count_tuple;
        using internal::is_columns;
        using internal::is_into;
        using internal::is_values;

        constexpr int intoArgsCount = count_tuple<args_tuple, is_into>::value;
        static_assert(intoArgsCount != 0, "Raw replace must have into<T> argument");
        static_assert(intoArgsCount < 2, "Raw replace must have only one into<T> argument");

        constexpr int columnsArgsCount = count_tuple<args_tuple, is_columns>::value;
        static_assert(columnsArgsCount < 2, "Raw replace must have only one columns(...) argument");

        constexpr int valuesArgsCount = count_tuple<args_tuple, is_values>::value;
        static_assert(valuesArgsCount < 2, "Raw replace must have only one values(...) argument");

        constexpr int defaultValuesCount = count_tuple<args_tuple, internal::is_default_values>::value;
        static_assert(defaultValuesCount < 2, "Raw replace must have only one default_values() argument");

        constexpr int selectsArgsCount = count_tuple<args_tuple, internal::is_select>::value;
        static_assert(selectsArgsCount < 2, "Raw replace must have only one select(...) argument");

        constexpr int argsCount = int(std::tuple_size<args_tuple>::value);
        static_assert(argsCount ==
                          intoArgsCount + columnsArgsCount + valuesArgsCount + defaultValuesCount + selectsArgsCount,
                      "Raw replace has invalid arguments");

        return {{std::forward<Args>(args)...}};
    }

    /**
     *  Create a replace range statement.
     *  The objects in the range are transformed using the specified projection, which defaults to identity projection.
     *
     *  @example
     *  ```
     *  std::vector<User> users;
     *  users.push_back(User{1, "Leony"});
     *  auto statement = storage.prepare(replace_range(users.begin(), users.end()));
     *  storage.execute(statement);
     *  ```
     *  @example
     *  ```
     *  std::vector<std::unique_ptr<User>> userPointers;
     *  userPointers.push_back(std::make_unique<User>(1, "Eneli"));
     *  auto statement = storage.prepare(replace_range(userPointers.begin(), userPointers.end(), &std::unique_ptr<User>::operator*));
     *  storage.execute(statement);
     *  ```
     */
    template<class It, class Projection = polyfill::identity>
    auto replace_range(It from, It to, Projection project = {}) {
        using O = std::decay_t<decltype(polyfill::invoke(std::declval<Projection>(), *std::declval<It>()))>;
        return internal::replace_range_t<It, Projection, O>{{std::move(from), std::move(to)}, std::move(project)};
    }

    /*
     *  Create a replace range statement.
     *  Overload of `replace_range(It, It, Projection)` with explicit object type template parameter.
     */
    template<class O, class It, class Projection = polyfill::identity>
    internal::replace_range_t<It, Projection, O> replace_range(It from, It to, Projection project = {}) {
        return {{std::move(from), std::move(to)}, std::move(project)};
    }

    /**
     *  Create an insert range statement.
     *  The objects in the range are transformed using the specified projection, which defaults to identity projection.
     *  
     *  @example
     *  ```
     *  std::vector<User> users;
     *  users.push_back(User{1, "Leony"});
     *  auto statement = storage.prepare(insert_range(users.begin(), users.end()));
     *  storage.execute(statement);
     *  ```
     *  @example
     *  ```
     *  std::vector<std::unique_ptr<User>> userPointers;
     *  userPointers.push_back(std::make_unique<User>(1, "Eneli"));
     *  auto statement = storage.prepare(insert_range(userPointers.begin(), userPointers.end(), &std::unique_ptr<User>::operator*));
     *  storage.execute(statement);
     *  ```
     */
    template<class It, class Projection = polyfill::identity>
    auto insert_range(It from, It to, Projection project = {}) {
        using O = std::decay_t<decltype(polyfill::invoke(std::declval<Projection>(), *std::declval<It>()))>;
        return internal::insert_range_t<It, Projection, O>{{std::move(from), std::move(to)}, std::move(project)};
    }

    /*
     *  Create an insert range statement.
     *  Overload of `insert_range(It, It, Projection)` with explicit object type template parameter.
     */
    template<class O, class It, class Projection = polyfill::identity>
    internal::insert_range_t<It, Projection, O> insert_range(It from, It to, Projection project = {}) {
        return {{std::move(from), std::move(to)}, std::move(project)};
    }

    /**
     *  Create a replace statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.replace(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.replace(std::ref(myUserInstance));
     */
    template<class T>
    internal::replace_t<T> replace(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an insert statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.insert(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.insert(std::ref(myUserInstance));
     */
    template<class T>
    internal::insert_t<T> insert(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create an explicit insert statement.
     *  T is an object type mapped to a storage.
     *  Cols is columns types aparameter pack. Must contain member pointers
     *  Usage: storage.insert(myUserInstance, columns(&User::id, &User::name));
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.insert(std::ref(myUserInstance), columns(&User::id, &User::name));
     */
    template<class T, class... Cols>
    internal::insert_explicit<T, Cols...> insert(T obj, internal::columns_t<Cols...> cols) {
        return {std::move(obj), std::move(cols)};
    }

    /**
     *  Create a remove statement
     *  T is an object type mapped to a storage.
     *  Usage: remove<User>(5);
     */
    template<class T, class... Ids>
    internal::remove_t<T, Ids...> remove(Ids... ids) {
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a remove statement
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: remove<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto remove(Ids... ids) {
        return remove<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create an update statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.update(myUserInstance);
     *  Parameter obj is accepted by value. If you want to accept it by ref
     *  please use std::ref function: storage.update(std::ref(myUserInstance));
     */
    template<class T>
    internal::update_t<T> update(T obj) {
        return {std::move(obj)};
    }

    /**
     *  Create a get statement.
     *  T is an object type mapped to a storage.
     *  Usage: get<User>(5);
     */
    template<class T, class... Ids>
    internal::get_t<T, Ids...> get(Ids... ids) {
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get(Ids... ids) {
        return get<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a get pointer statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_pointer<User>(5);
     */
    template<class T, class... Ids>
    internal::get_pointer_t<T, Ids...> get_pointer(Ids... ids) {
        return {{std::forward<Ids>(ids)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get pointer statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get_pointer<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get_pointer(Ids... ids) {
        return get_pointer<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    /**
     *  Create a get optional statement.
     *  T is an object type mapped to a storage.
     *  Usage: get_optional<User>(5);
     */
    template<class T, class... Ids>
    internal::get_optional_t<T, Ids...> get_optional(Ids... ids) {
        return {{std::forward<Ids>(ids)...}};
    }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get optional statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: get_optional<user_table>(5);
     */
    template<orm_table_reference auto table, class... Ids>
    auto get_optional(Ids... ids) {
        return get_optional<internal::auto_decay_table_ref_t<table>>(std::forward<Ids>(ids)...);
    }
#endif

    /**
     *  Create a remove all statement.
     *  T is an object type mapped to a storage.
     *  Usage: storage.remove_all<User>(...);
     */
    template<class T, class... Args>
    internal::remove_all_t<T, Args...> remove_all(Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        return {{std::forward<Args>(args)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a remove all statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  Usage: storage.remove_all<user_table>(...);
     */
    template<orm_table_reference auto table, class... Args>
    auto remove_all(Args... args) {
        return remove_all<internal::auto_decay_table_ref_t<table>>(std::forward<Args>(args)...);
    }
#endif

    /**
     *  Create a get all statement.
     *  T is an explicitly specified object mapped to a storage or a table alias.
     *  R is a container type. std::vector<T> is default
     *  Usage: storage.prepare(get_all<User>(...));
     */
    template<class T, class R = std::vector<internal::mapped_type_proxy_t<T>>, class... Args>
    internal::get_all_t<T, R, Args...> get_all(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_conditions<conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all statement.
     *  `mapped` is an explicitly specified table reference or table alias to be extracted.
     *  `R` is the container return type, which must have a `R::push_back(T&&)` method, and defaults to `std::vector<T>`
     *  Usage: storage.get_all<sqlite_schema>(...);
     */
    template<orm_refers_to_table auto mapped,
             class R = std::vector<internal::mapped_type_proxy_t<decltype(mapped)>>,
             class... Args>
    auto get_all(Args && ... conditions) {
        return get_all<internal::auto_decay_table_ref_t<mapped>, R>(std::forward<Args>(conditions)...);
    }
#endif

    /**
     *  Create an update all statement.
     *  Usage: storage.update_all(set(...), ...);
     */
    template<class S, class... Wargs>
    internal::update_all_t<S, Wargs...> update_all(S set, Wargs... wh) {
        static_assert(internal::is_set<S>::value, "first argument in update_all can be either set or dynamic_set");
        using args_tuple = std::tuple<Wargs...>;
        internal::validate_conditions<args_tuple>();
        return {std::move(set), {std::forward<Wargs>(wh)...}};
    }

    /**
     *  Create a get all pointer statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::unique_ptr<T>> is default
     *  Usage: storage.prepare(get_all_pointer<User>(...));
     */
    template<class T, class R = std::vector<std::unique_ptr<T>>, class... Args>
    internal::get_all_pointer_t<T, R, Args...> get_all_pointer(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_conditions<conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all pointer statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  R is a container return type. std::vector<std::unique_ptr<T>> is default
     *  Usage: storage.prepare(get_all_pointer<user_table>(...));
     */
    template<orm_table_reference auto table,
             class R = std::vector<internal::auto_decay_table_ref_t<table>>,
             class... Args>
    auto get_all_pointer(Args... conditions) {
        return get_all_pointer<internal::auto_decay_table_ref_t<table>, R>(std::forward<Args>(conditions)...);
    }
#endif

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    /**
     *  Create a get all optional statement.
     *  T is an object type mapped to a storage.
     *  R is a container return type. std::vector<std::optional<T>> is default
     *  Usage: storage.get_all_optional<User>(...);
     */
    template<class T, class R = std::vector<std::optional<T>>, class... Args>
    internal::get_all_optional_t<T, R, Args...> get_all_optional(Args... conditions) {
        using conditions_tuple = std::tuple<Args...>;
        internal::validate_conditions<conditions_tuple>();
        return {{std::forward<Args>(conditions)...}};
    }
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Create a get all optional statement.
     *  `table` is an explicitly specified table reference of a mapped object to be extracted.
     *  R is a container return type. std::vector<std::optional<T>> is default
     *  Usage: storage.get_all_optional<user_table>(...);
     */
    template<orm_table_reference auto table,
             class R = std::vector<internal::auto_decay_table_ref_t<table>>,
             class... Args>
    auto get_all_optional(Args && ... conditions) {
        return get_all_optional<internal::auto_decay_table_ref_t<table>, R>(std::forward<Args>(conditions)...);
    }
#endif
}
