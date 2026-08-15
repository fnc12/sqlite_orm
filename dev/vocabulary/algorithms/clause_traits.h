#pragma once

/** @file Combined grammar traits classifying statement-level clauses of a select statement.
 *
 *  A statement-level clause (FROM, JOIN, WHERE, GROUP BY, WINDOW, ORDER BY, LIMIT) may only
 *  appear in the conditions pack of a statement, in the canonical clause order, and may only
 *  hold expressions - the serializer streams clauses positionally, so any other arrangement
 *  would generate invalid SQL.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::enable_if, std::true_type
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
    /**
     *  Rank of a statement-level clause in the canonical SQL clause order,
     *  or 0 if the type is not a statement-level clause.
     */
    template<class T>
    constexpr int clause_rank_v = polyfill::disjunction<is_from<T>, is_from2<T>>::value ? 1
                                  : is_any_join<T>::value                               ? 2
                                  : is_where<T>::value                                  ? 3
                                  : is_group_by<T>::value                               ? 4
                                  : is_window_defn<T>::value                            ? 5
                                  : is_order_by<T>::value                               ? 6
                                  : is_limit<T>::value                                  ? 7
                                                                                        : 0;

    template<class T>
    using is_statement_clause = polyfill::bool_constant<clause_rank_v<T> != 0>;

    template<class Tpl>
    struct tuple_holds_no_clause;

    template<template<class...> class Tpl, class... Args>
    struct tuple_holds_no_clause<Tpl<Args...>> : polyfill::bool_constant<((clause_rank_v<Args> == 0) && ...)> {};

    template<class Tpl>
    struct tuple_of_order_by_terms;

    template<template<class...> class Tpl, class... Args>
    struct tuple_of_order_by_terms<Tpl<Args...>>
        : polyfill::conjunction<
              polyfill::conjunction<is_order_by<Args>, polyfill::is_detected<expression_type_t, Args>>...> {};

    /**
     *  Checks that the expressions nested in a clause are not statement-level clauses themselves:
     *  expressions like `where(group_by(...))` or `order_by(limit(...))` would generate invalid SQL.
     */
    template<class T, class SFINAE = void>
    struct clause_holds_no_clause : std::true_type {};

    //  clauses carrying a single expression: WHERE, a single ORDER BY term
    template<class T>
    struct clause_holds_no_clause<
        T,
        std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<is_where<T>, is_order_by<T>>,
                                                 polyfill::is_detected<expression_type_t, T>>>>
        : polyfill::bool_constant<clause_rank_v<polyfill::detected_or_t<void, expression_type_t, T>> == 0> {};

    //  ORDER BY with multiple terms: every term must be a single ORDER BY term
    template<class T>
    struct clause_holds_no_clause<
        T,
        std::enable_if_t<polyfill::conjunction_v<is_order_by<T>, polyfill::is_detected<args_type_t, T>>>>
        : tuple_of_order_by_terms<polyfill::detected_or_t<std::tuple<>, args_type_t, T>> {};

    //  GROUP BY carries a tuple of expressions and possibly a HAVING expression
    template<class T>
    struct clause_holds_no_clause<T, std::enable_if_t<is_group_by_v<T>>>
        : polyfill::conjunction<
              tuple_holds_no_clause<polyfill::detected_or_t<std::tuple<>, args_type_t, T>>,
              polyfill::bool_constant<clause_rank_v<polyfill::detected_or_t<void, expression_type_t, T>> == 0>> {};

    //  LIMIT carries the limit and possibly an offset expression
    template<class T>
    struct clause_holds_no_clause<T, std::enable_if_t<is_limit_v<T>>>
        : polyfill::bool_constant<clause_rank_v<polyfill::detected_or_t<void, expression_type_t, T>> == 0 &&
                                  clause_rank_v<polyfill::detected_or_t<void, offset_expression_type_t, T>> == 0> {};
}
