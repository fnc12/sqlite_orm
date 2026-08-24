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
#include <initializer_list>
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/mpl.h"
#include "../../tuple_helper/tuple_traits.h"
#include "../node_traits.h"

// generic statement clause algorithms
namespace sqlite_orm::internal {
    /**
     *  Position of the first clause trait satisfied by `T` within the given list of clause traits,
     *  or `sizeof...(Clause)` if `T` satisfies none of them.
     */
    template<class T, template<class...> class... Clause>
    constexpr size_t clause_position_v = find_tuple_satisfied_by<mpl::pack<mpl::quote_fn<Clause>...>, T>::value;

    /**
     *  Rank of `T` as a statement-level clause, in the canonical order given by the list of clause traits,
     *  or 0 if `T` is not one of those clauses.
     *
     *  Implementation note: this is deliberately not a predicate but an ordinal projection - it is the
     *  mechanism both `is_..._clause` and the clause order check are built on. The traits are listed in
     *  the order the clauses must be streamed in, so the rank of a clause is its 1-based position in
     *  that list; the miss position maps to 0 so that "not a clause" and "first clause" stay distinct.
     */
    template<class T, template<class...> class... Clause>
    constexpr size_t clause_rank_v =
        clause_position_v<T, Clause...> == sizeof...(Clause) ? 0 : clause_position_v<T, Clause...> + 1;

    /*
     *  Checks that clause ranks are in non-descending order, i.e. that no clause is preceded by a clause
     *  of higher rank. Equal ranks are allowed, since a statement may repeat a clause - e.g. several JOINs.
     */
    constexpr bool clause_ranks_are_ordered(std::initializer_list<size_t> ranks) {
        size_t lastRank = 0;
        for (size_t rank: ranks) {
            if (rank < lastRank) {
                return false;
            }
            lastRank = rank;
        }
        return true;
    }

    /**
     *  Checks that the clauses in a pack appear in the canonical order of the statement they belong to.
     *
     *  `RankOp` is a metafunction yielding the rank of a clause, e.g. `select_clause_rank`.
     */
    template<class Pack, template<class...> class RankOp>
    constexpr bool clauses_are_correctly_ordered_v = false;

    template<template<class...> class Pack, class... T, template<class...> class RankOp>
    constexpr bool clauses_are_correctly_ordered_v<Pack<T...>, RankOp> =
        clause_ranks_are_ordered({RankOp<T>::value...});
}

// select statement clause algorithms
namespace sqlite_orm::internal {
    /**
     *  Rank of a statement-level clause as part of the select-core and tail of the factored-select-stmt,
     *  or 0 if the type is not a statement-level clause.
     */
    template<class T>
    constexpr size_t select_clause_rank_v = clause_rank_v<T,
                                                          mpl::disjunction_fn<is_from, is_from2>::template fn,
                                                          is_any_join,
                                                          is_where,
                                                          is_group_by,
                                                          is_window_defn,
                                                          is_order_by,
                                                          is_limit>;

    /*
     *  Implementation note: a derived struct in favor of an alias template, because it is passed on as a
     *  template-template argument - type replacement of an alias template having a non-type template parameter
     *  from a dependent expression in it may fail [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR].
     */
    template<class T>
    struct select_clause_rank : polyfill::index_constant<select_clause_rank_v<T>> {};

    template<class T>
    using is_select_clause = polyfill::bool_constant<select_clause_rank_v<T> != 0>;

    /**
     *  Checks that the clauses in the conditions pack of a select statement are listed
     *  in the canonical clause order.
     */
    template<class Tpl>
    constexpr bool check_select_clause_order_v = clauses_are_correctly_ordered_v<Tpl, select_clause_rank>;

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
