#pragma once

/** @file The INSERT statement, in each of the DSL spellings sqlite_orm offers for it - against a
 *        mapped object, against a range of them, against an explicit column list, or raw -, plus
 *        the OR conflict-resolution modifier only a raw INSERT takes.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::conjunction, std::is_same, std::is_convertible, std::declval
#include <utility>  //  std::move, std::forward, std::pair
#include <tuple>  //  std::tuple, std::tuple_size
#include <functional>  //  std::invoke
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/cxx_functional_polyfill.h"  //  polyfill::identity
#include "../../tuple_helper/tuple_traits.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits
#include "../result_columns.h"  // columns_t

namespace sqlite_orm::internal {
    template<class T>
    struct insert_t {
        using object_type = T;

        object_type object;
    };

    template<class T>
    constexpr bool is_insert_v = polyfill::is_specialization_of<T, insert_t>::value;

    template<class T>
    constexpr bool is_object_dml_expression_v<T, std::enable_if_t<is_insert_v<T>>> = true;

    template<class T, class... Cols>
    struct insert_explicit {
        using object_type = T;
        using columns_type = columns_t<Cols...>;

        object_type object;
        columns_type columns;
    };

    template<class T>
    constexpr bool is_insert_explicit_v = polyfill::is_specialization_of<T, insert_explicit>::value;

    template<class T>
    constexpr bool is_object_dml_expression_v<T, std::enable_if_t<is_insert_explicit_v<T>>> = true;

    template<class It, class Projection, class O>
    struct insert_range_t {
        using iterator_type = It;
        using transformer_type = Projection;
        using object_type = O;

        std::pair<iterator_type, iterator_type> range;
        transformer_type transformer;
    };

    template<class T>
    constexpr bool is_insert_range_v = polyfill::is_specialization_of<T, insert_range_t>::value;

    template<class... Args>
    struct insert_raw_t {
        using args_tuple = std::tuple<Args...>;

        args_tuple args;
    };

    template<class T>
    constexpr bool is_insert_raw_v = polyfill::is_specialization_of<T, insert_raw_t>::value;

    template<class T>
    constexpr bool is_raw_dml_expression_v<T, std::enable_if_t<is_insert_raw_v<T>>> = true;

    template<class With>
    constexpr bool is_raw_dml_expression_v<
        With,
        std::enable_if_t<std::conjunction_v<is_with_clause<With>, is_insert_raw<expression_type_t<With>>>>> = true;

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
    constexpr bool is_insert_constraint_v = std::is_same<T, insert_constraint>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
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
        using internal::is_any_values;
        using internal::is_columns;
        using internal::is_default_values;
        using internal::is_insert_constraint;
        using internal::is_into;
        using internal::is_select;
        using internal::is_upsert_clause;

        constexpr int orArgsCount = count_tuple<args_tuple, is_insert_constraint>::value;
        static_assert(orArgsCount < 2, "Raw insert must have only one OR... argument");

        constexpr int intoArgsCount = count_tuple<args_tuple, is_into>::value;
        static_assert(intoArgsCount != 0, "Raw insert must have into<T> argument");
        static_assert(intoArgsCount < 2, "Raw insert must have only one into<T> argument");

        constexpr int columnsArgsCount = count_tuple<args_tuple, is_columns>::value;
        static_assert(columnsArgsCount < 2, "Raw insert must have only one columns(...) argument");

        constexpr int valuesArgsCount = count_tuple<args_tuple, is_any_values>::value;
        static_assert(valuesArgsCount < 2, "Raw insert must have only one values(...) argument");

        constexpr int defaultValuesCount = count_tuple<args_tuple, is_default_values>::value;
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
        using O = std::decay_t<decltype(std::invoke(std::declval<Projection>(), *std::declval<It>()))>;
        return internal::insert_range_t<It, Projection, O>{{std::move(from), std::move(to)}, std::move(project)};
    }

    /*
     *  Create an insert range statement.
     *  Overload of `insert_range(It, It, Projection)` with explicit object type template parameter.
     */
    template<class O, class It, class Projection = polyfill::identity>
    internal::insert_range_t<It, Projection, O> insert_range(It from, It to, Projection project = {}) {
        // validate up front that projected type is convertible to mapped object type, avoiding hard to read error messages later;
        // note: we use `is_convertible` instead of `is_invocable_r` because we do not create dangling references in `storage_t<>::execute()`
        using projected_type = decltype(std::invoke(std::declval<Projection>(), *std::declval<It>()));
        static_assert(std::is_convertible<projected_type, const O&>::value,
                      "Projected type must be convertible to mapped object type");

        return {{std::move(from), std::move(to)}, std::move(project)};
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
}
