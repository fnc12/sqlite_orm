#pragma once

/** @file
 *  Purpose: decide which nodes are admissible elements of a compound expression construct.
 *
 *  Intention: an algorithm of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md` holds; composes
 *  the grammar traits into the element checks of the expression constructs of `ast/`.
 */

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../node_traits.h"

// window definitions
namespace sqlite_orm::internal {
    /**
     *  Whether a node is an admissible element of a window definition: PARTITION BY, ORDER BY,
     *  and a frame specification.
     */
    template<class T>
    constexpr bool is_window_defn_element_v =
        std::disjunction_v<is_partition_by<T>, is_any_order_by<T>, is_frame_spec<T>>;

    /**
     *  Whether a pack forms the arguments of an OVER clause: either a lone reference to a named
     *  window - `OVER name` - or the elements of an inline window definition, of which the empty
     *  definition `OVER ()` is one.
     *
     *  A window reference is admissible only on its own. SQLite's base-window-name form,
     *  `OVER (name PARTITION BY ...)`, has no spelling in the DSL, and the serializer streams a
     *  window reference only as the sole argument.
     */
    template<class... Args>
    constexpr bool are_valid_over_arguments_v =
        (sizeof...(Args) == 1 && (is_window_ref_v<Args> && ...)) || (is_window_defn_element_v<Args> && ...);
}

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
        std::disjunction_v<is_unbounded_preceding<T>, is_preceding<T>, is_current_row<T>, is_following<T>>;

    /**
     *  Whether a node may close a window frame: the frame cannot end with UNBOUNDED PRECEDING.
     */
    template<class T>
    constexpr bool is_frame_end_bound_v =
        std::disjunction_v<is_preceding<T>, is_current_row<T>, is_following<T>, is_unbounded_following<T>>;
}
