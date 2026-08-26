#pragma once

/** @file Closed predicates classifying the statement-level clauses of a statement by type and order.
 *
 *  A statement-level clause may only appear in the conditions pack of a statement, and only in the
 *  canonical clause order - the serializer streams clauses positionally, so any other arrangement
 *  would generate invalid SQL. That a clause holds an expression rather than another clause is
 *  enforced by the clause factories, on the argument each is handed.
 *
 *  The generic algorithms take the clauses of a statement as an ordered list of clause traits,
 *  or as the rank metafunction derived from it, so that a statement kind is expressed by declaring
 *  its own list. The select statement (FROM, JOIN, WHERE, GROUP BY, WINDOW, ORDER BY, LIMIT)
 *  is the first such list.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
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
    constexpr size_t select_clause_rank_v =
        clause_rank_v<T, is_any_from, is_any_join, is_where, is_group_by, is_window_defn, is_order_by, is_limit>;

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
}

// delete statement clause algorithms
namespace sqlite_orm::internal {
    /**
     *  Rank of a statement-level clause of a DELETE statement, or 0 if the type is not one of them.
     *
     *  ORDER BY and LIMIT are only accepted by an SQLite built with SQLITE_ENABLE_UPDATE_DELETE_LIMIT.
     *  That is a build option of the library we link against and cannot be detected from here, so they
     *  are admitted and left to SQLite to reject.
     */
    template<class T>
    constexpr size_t delete_clause_rank_v = clause_rank_v<T, is_where, is_order_by, is_limit>;

    //  a derived struct in favor of an alias template, because it is passed on as a template-template argument
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
    template<class T>
    struct delete_clause_rank : polyfill::index_constant<delete_clause_rank_v<T>> {};

    template<class T>
    using is_delete_clause = polyfill::bool_constant<delete_clause_rank_v<T> != 0>;

    /**
     *  Checks that the clauses in the conditions pack of a delete statement are listed
     *  in the canonical clause order.
     */
    template<class Tpl>
    constexpr bool check_delete_clause_order_v = clauses_are_correctly_ordered_v<Tpl, delete_clause_rank>;
}

// update statement clause algorithms
namespace sqlite_orm::internal {
    /**
     *  Rank of a statement-level clause of an UPDATE statement, or 0 if the type is not one of them.
     *
     *  The FROM clause of `UPDATE ... SET ... FROM ...`, and the joins it may carry, exist as of SQLite 3.33.0.
     *  For ORDER BY and LIMIT the same applies as for a delete statement.
     */
    template<class T>
    constexpr size_t update_clause_rank_v = clause_rank_v<T,
#if (SQLITE_VERSION_NUMBER >= 3033000)
                                                          is_any_from,
                                                          is_any_join,
#endif
                                                          is_where,
                                                          is_order_by,
                                                          is_limit>;

    //  a derived struct in favor of an alias template, because it is passed on as a template-template argument
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
    template<class T>
    struct update_clause_rank : polyfill::index_constant<update_clause_rank_v<T>> {};

    template<class T>
    using is_update_clause = polyfill::bool_constant<update_clause_rank_v<T> != 0>;

    /**
     *  Checks that the clauses in the conditions pack of an update statement are listed
     *  in the canonical clause order.
     */
    template<class Tpl>
    constexpr bool check_update_clause_order_v = clauses_are_correctly_ordered_v<Tpl, update_clause_rank>;
}

// clauses of any statement kind
namespace sqlite_orm::internal {
    /**
     *  Whether a node is a statement-level clause of any statement kind.
     *
     *  The delete and update clause lists are subsets of the select one, so the select list is presently
     *  the union. Extend this should a statement kind ever admit a clause the select statement does not.
     *  It carries its own name because a clause factory has to reject a nested clause regardless of which
     *  statement its result will end up in.
     */
    template<class T>
    using is_statement_clause = is_select_clause<T>;
}
