#pragma once

/** @file The UPDATE statement, in both of the DSL spellings sqlite_orm offers for it - against a
 *        mapped object, or raw against a SET clause and conditions.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::conjunction
#include <utility>  //  std::move, std::forward
#include <tuple>  //  std::tuple, std::tuple_size
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../tuple_helper/tuple_traits.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/node_algorithms.h"  // clause predicates
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct update_t {
        using object_type = T;

        object_type object;
    };

    template<class T>
    constexpr bool is_update_v = polyfill::is_specialization_of<T, update_t>::value;

    template<class T>
    constexpr bool is_object_dml_expression_v<T, std::enable_if_t<is_update_v<T>>> = true;

    template<class S, class... Wargs>
    struct update_all_t {
        using set_type = S;
        using conditions_type = std::tuple<Wargs...>;

        static_assert(is_any_set<S>::value, "update_all_t must have set or dynamic set as the first argument");

        set_type set;
        conditions_type conditions;
    };

    template<class T>
    constexpr bool is_update_all_v = polyfill::is_specialization_of<T, update_all_t>::value;

    template<class T>
    constexpr bool is_raw_dml_expression_v<T, std::enable_if_t<is_update_all_v<T>>> = true;

    template<class With>
    constexpr bool is_raw_dml_expression_v<
        With,
        std::enable_if_t<std::conjunction_v<is_with_clause<With>, is_update_all<expression_type_t<With>>>>> = true;

    /**
     *  The update statement counterpart of `validate_select_clauses()`; see there for the split of
     *  responsibilities between this and the clause factories.
     */
    template<class T>
    constexpr void validate_update_clauses() {
        static_assert(count_tuple<T, is_any_from>::value <= 1, "a single statement cannot contain > 1 FROM blocks");
        static_assert(count_tuple<T, is_where>::value <= 1, "a single statement cannot contain > 1 WHERE blocks");
        static_assert(count_tuple<T, is_any_order_by>::value <= 1,
                      "a single statement cannot contain > 1 ORDER BY blocks");
        static_assert(count_tuple<T, is_limit>::value <= 1, "a single statement cannot contain > 1 LIMIT blocks");
        static_assert(std::tuple_size<T>::value == count_tuple<T, is_update_clause>::value,
                      "an UPDATE argument must be a FROM, JOIN, WHERE, ORDER BY or LIMIT clause");
        static_assert(check_update_clause_order_v<T>,
                      "SQL clauses must be listed in the canonical order: FROM, JOINs, WHERE, ORDER BY, LIMIT");
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
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
     *  Create an update all statement.
     *  Usage: storage.update_all(set(...), ...);
     */
    template<class S, class... Wargs>
    internal::update_all_t<S, Wargs...> update_all(S set, Wargs... wh) {
        static_assert(internal::is_any_set<S>::value, "first argument in update_all can be either set or dynamic_set");
        using args_tuple = std::tuple<Wargs...>;
        internal::validate_update_clauses<args_tuple>();
        return {std::move(set), {std::forward<Wargs>(wh)...}};
    }
}
