#pragma once

/** @file
 *  Purpose: compute the per-column types of a single table definition.
 *
 *  Intention: an algorithm of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md` holds; composes
 *  the grammar traits and the projections over the elements of a table definition.
 */

#include "../../functional/mpl/convenience.h"
#include "../../functional/tuple_helper/tuple_filter.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
    /**
     *  The field types of a table definition's columns, in column order.
     */
    template<class Table>
    using column_field_types_t = transform_tuple<filter_tuple_t<elements_type_t<Table>, is_column>, field_type_t>;

    /**
     *  The member pointers a table definition's columns are mapped by, in column order.
     */
    template<class Table>
    using column_field_expressions_t =
        transform_tuple<filter_tuple_t<elements_type_t<Table>, is_column>, member_pointer_type_t>;
}
