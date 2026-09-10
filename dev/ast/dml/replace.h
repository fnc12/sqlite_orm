#pragma once

/** @file The REPLACE statement, in each of the DSL spellings sqlite_orm offers for it - against a
 *        mapped object, against a range of them, or raw.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::conjunction, std::is_convertible, std::declval
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

namespace sqlite_orm::internal {
    template<class T>
    struct replace_t {
        using object_type = T;

        object_type object;
    };

    template<class T>
    constexpr bool is_replace_v = polyfill::is_specialization_of<T, replace_t>::value;

    template<class T>
    constexpr bool is_object_dml_expression_v<T, std::enable_if_t<is_replace_v<T>>> = true;

    template<class It, class Projection, class O>
    struct replace_range_t {
        using iterator_type = It;
        using transformer_type = Projection;
        using object_type = O;

        std::pair<iterator_type, iterator_type> range;
        transformer_type transformer;
    };

    template<class T>
    constexpr bool is_replace_range_v = polyfill::is_specialization_of<T, replace_range_t>::value;

    template<class... Args>
    struct replace_raw_t {
        using args_tuple = std::tuple<Args...>;

        args_tuple args;
    };

    template<class T>
    constexpr bool is_replace_raw_v = polyfill::is_specialization_of<T, replace_raw_t>::value;

    template<class T>
    constexpr bool is_raw_dml_expression_v<T, std::enable_if_t<is_replace_raw_v<T>>> = true;

    template<class With>
    constexpr bool is_raw_dml_expression_v<
        With,
        std::enable_if_t<std::conjunction_v<is_with_clause<With>, is_replace_raw<expression_type_t<With>>>>> = true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
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
        using internal::is_default_values;
        using internal::is_into;
        using internal::is_select;
        using internal::is_values;

        constexpr int intoArgsCount = count_tuple<args_tuple, is_into>::value;
        static_assert(intoArgsCount != 0, "Raw replace must have into<T> argument");
        static_assert(intoArgsCount < 2, "Raw replace must have only one into<T> argument");

        constexpr int columnsArgsCount = count_tuple<args_tuple, is_columns>::value;
        static_assert(columnsArgsCount < 2, "Raw replace must have only one columns(...) argument");

        constexpr int valuesArgsCount = count_tuple<args_tuple, is_values>::value;
        static_assert(valuesArgsCount < 2, "Raw replace must have only one values(...) argument");

        constexpr int defaultValuesCount = count_tuple<args_tuple, is_default_values>::value;
        static_assert(defaultValuesCount < 2, "Raw replace must have only one default_values() argument");

        constexpr int selectsArgsCount = count_tuple<args_tuple, is_select>::value;
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
        using O = std::decay_t<decltype(std::invoke(std::declval<Projection>(), *std::declval<It>()))>;
        return internal::replace_range_t<It, Projection, O>{{std::move(from), std::move(to)}, std::move(project)};
    }

    /*
     *  Create a replace range statement.
     *  Overload of `replace_range(It, It, Projection)` with explicit object type template parameter.
     */
    template<class O, class It, class Projection = polyfill::identity>
    internal::replace_range_t<It, Projection, O> replace_range(It from, It to, Projection project = {}) {
        // validate up front that projected type is convertible to mapped object type, avoiding hard to read error messages later;
        // note: we use `is_convertible` instead of `is_invocable_r` because we do not create dangling references in `storage_t<>::execute()`
        using projected_type = decltype(std::invoke(std::declval<Projection>(), *std::declval<It>()));
        static_assert(std::is_convertible<projected_type, const O&>::value,
                      "Projected type must be convertible to mapped object type");

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
}
