#pragma once

/** @file Closed alias templates computing per-column type tuples of a single table definition.
 */

#include "../../tuple_helper/tuple_filter.h"
#include "../../tuple_helper/tuple_transformer.h"
#include "../node_traits.h"

namespace sqlite_orm::internal {
    /**
     *  The field types of a table definition's columns, in column order.
     */
    template<class Table>
    using column_field_types_t = transform_tuple_t<filter_tuple_t<elements_type_t<Table>, is_column>, field_type_t>;

    /**
     *  The member pointers a table definition's columns are mapped by, in column order.
     */
    template<class Table>
    using column_field_expressions_t =
        transform_tuple_t<filter_tuple_t<elements_type_t<Table>, is_column>, member_pointer_type_t>;
}
