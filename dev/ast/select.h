#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple, std::tuple_size
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tuple_helper/tuple_traits.h"
#include "../vocabulary/node_algorithms.h"  // clause predicates
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  Subselect object type.
     */
    template<class T, class... Args>
    struct select_t {
        using return_type = T;
        using conditions_type = std::tuple<Args...>;

        return_type col;
        conditions_type conditions;
        bool highest_level = false;
    };

    template<class T>
    constexpr bool is_select_v = polyfill::is_specialization_of<T, select_t>::value;

    template<class Select>
    constexpr bool is_select_expression_v<Select, std::enable_if_t<is_select_v<Select>>> = true;

    /**
     *  Checks the relations that only the conditions pack as a whole can answer: how often a clause may
     *  appear in it, that every argument is a clause at all, and that they are listed in the canonical order.
     *  Whether a single clause holds an admissible expression is checked by the clause factory that built it.
     */
    template<class T>
    constexpr void validate_select_clauses() {
        static_assert(count_tuple<T, is_where>::value <= 1, "a single query cannot contain > 1 WHERE blocks");
        static_assert(count_tuple<T, is_group_by>::value <= 1, "a single query cannot contain > 1 GROUP BY blocks");
        static_assert(count_tuple<T, is_order_by>::value <= 1, "a single query cannot contain > 1 ORDER BY blocks");
        static_assert(count_tuple<T, is_limit>::value <= 1, "a single query cannot contain > 1 LIMIT blocks");
        static_assert(count_tuple<T, is_any_from>::value <= 1, "a single query cannot contain > 1 FROM blocks");
        static_assert(std::tuple_size<T>::value == count_tuple<T, is_select_clause>::value,
                      "a query argument must be a FROM, JOIN, WHERE, GROUP BY, WINDOW, ORDER BY or LIMIT clause");
        static_assert(check_select_clause_order_v<T>,
                      "SQL clauses must be listed in the canonical order: FROM, JOINs, WHERE, GROUP BY, WINDOW, "
                      "ORDER BY, LIMIT");
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Public function for subselect query. Is useful in UNION queries.
     */
    template<class T, class... Args>
    constexpr internal::select_t<T, Args...> select(T t, Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_select_clauses<args_tuple>();
        return {std::move(t), {std::forward<Args>(args)...}};
    }
}
