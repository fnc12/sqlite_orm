#pragma once

/** @file Closed predicates classifying the statement-level clauses of a select statement by type and order.
 *
 *  A statement-level clause (FROM, JOIN, WHERE, GROUP BY, WINDOW, ORDER BY, LIMIT) may only
 *  appear in the conditions pack of a statement, in the canonical clause order, and may only
 *  hold expressions - the serializer streams clauses positionally, so any other arrangement
 *  would generate invalid SQL.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple_size
#include <type_traits>  //  std::enable_if
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../tuple_helper/tuple_traits.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
    /**
     *  Rank of a statement-level clause as part of the select-core and tail of the factored-select-stmt,
     *  or 0 if the type is not a statement-level clause.
     */
    template<class T>
    constexpr int select_clause_rank_v = polyfill::disjunction<is_from<T>, is_from2<T>>::value ? 1
                                         : is_any_join<T>::value                               ? 2
                                         : is_where<T>::value                                  ? 3
                                         : is_group_by<T>::value                               ? 4
                                         : is_window_defn<T>::value                            ? 5
                                         : is_order_by<T>::value                               ? 6
                                         : is_limit<T>::value                                  ? 7
                                                                                               : 0;

    template<class T>
    using is_select_clause = polyfill::bool_constant<select_clause_rank_v<T> != 0>;

    /**
     *  Checks that the expressions nested in a clause are not statement-level clauses themselves:
     *  expressions like `where(group_by(...))` or `order_by(limit(...))` would generate invalid SQL.
     */
    template<class T, class SFINAE = void>
    constexpr bool select_clause_nests_no_clause_v = true;

    template<class T>
    using select_clause_nests_no_clause = polyfill::bool_constant<select_clause_nests_no_clause_v<T>>;

    //  clauses carrying a single expression: WHERE, a single ORDER BY term
    template<class T>
    constexpr bool select_clause_nests_no_clause_v<
        T,
        std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<is_where<T>, is_order_by<T>>,
                                                 polyfill::is_detected<expression_type_t, T>>>> =
        polyfill::negation_v<is_select_clause<expression_type_t<T>>>;

    //  ORDER BY with multiple terms: every term must be a single ORDER BY term
    template<class T>
    constexpr bool select_clause_nests_no_clause_v<
        T,
        std::enable_if_t<polyfill::conjunction_v<is_order_by<T>, polyfill::is_detected<args_type_t, T>>>> =
        mpl::invoke_t<mpl::counts<mpl::conjunction<mpl::quote_fn<is_order_by>,
                                                   check_if_names<expression_type_t>,
                                                   check_if<select_clause_nests_no_clause>>>,
                      args_type_t<T>>::value == std::tuple_size<args_type_t<T>>::value;

    //  GROUP BY carries a tuple of expressions and possibly a HAVING expression
    template<class T>
    constexpr bool select_clause_nests_no_clause_v<T, std::enable_if_t<is_group_by_v<T>>> =
        polyfill::negation_v<polyfill::disjunction<tuple_has<args_type_t<T>, is_select_clause>,
                                                   // HAVING expression
                                                   is_select_clause<polyfill::detected_t<expression_type_t, T>>>>;

    //  LIMIT carries the limit and possibly an offset expression
    template<class T>
    constexpr bool select_clause_nests_no_clause_v<T, std::enable_if_t<is_limit_v<T>>> = polyfill::negation_v<
        polyfill::disjunction<is_select_clause<expression_type_t<T>>, is_select_clause<offset_expression_type_t<T>>>>;
}
