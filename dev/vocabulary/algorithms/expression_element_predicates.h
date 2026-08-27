#pragma once

/** @file Closed predicates for checking the membership of the elements of a compound expression construct.
 *
 *  These answer whether a node may occupy a given slot of an expression-level construct - a window
 *  definition and its frame, and the like - as opposed to the elements of a schema definition
 *  (`ddl_predicates.h`) or the clauses of a statement (`clause_predicates.h`).
 *
 *  That an element holds an expression rather than a statement clause is not checked here -
 *  the element factories enforce that on the argument each is handed.
 */

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../node_traits.h"

// window frame boundaries
namespace sqlite_orm::internal {
    /*
     *  Every frame boundary node is a boundary, but not at both ends of the frame: SQLite rejects a frame
     *  starting with UNBOUNDED FOLLOWING, and one ending with UNBOUNDED PRECEDING, as syntax errors.
     *  The frame factories `rows()`, `range()` and `groups()` check the boundaries they are handed.
     */

    /**
     *  Whether a node may open a window frame: the frame cannot start with UNBOUNDED FOLLOWING.
     */
    template<class T>
    constexpr bool is_frame_start_bound_v =
        polyfill::disjunction_v<is_unbounded_preceding<T>, is_preceding<T>, is_current_row<T>, is_following<T>>;

    /**
     *  Whether a node may close a window frame: the frame cannot end with UNBOUNDED PRECEDING.
     */
    template<class T>
    constexpr bool is_frame_end_bound_v =
        polyfill::disjunction_v<is_preceding<T>, is_current_row<T>, is_following<T>, is_unbounded_following<T>>;
}
