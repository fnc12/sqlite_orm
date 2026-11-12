#pragma once

/** @file
 *  Purpose: compute the fields of columns and the types that go with them.
 *
 *  Intention: an algorithm of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md` holds; composes
 *  the grammar traits and the projections over the elements of a table definition.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::make_tuple, std::get
#include <utility>  //  std::index_sequence
#endif

#include "../../functional/mpl/convenience.h"
#include "../../functional/tuple_helper/tuple_filter.h"
#include "../node_traits.h"
#include "index_filters.h"  //  col_index_sequence_of

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

    template<class Elements, size_t... Idx>
    auto get_table_column_fields(const Elements& elements, std::index_sequence<Idx...>) {
        return std::make_tuple(std::get<Idx>(elements).member_pointer...);
    }

    /**
     *  Return a tuple of the member pointers of all columns of a table.
     */
    template<class Table>
    auto get_table_column_fields(const Table& table) {
        return get_table_column_fields(table.elements, col_index_sequence_of<elements_type_t<Table>>{});
    }
}
