#pragma once

/** @file Closed classifier traits of DSL-only nodes with no SQL grammar counterpart.
 *        E.g. ad-hoc column-selection structs, mapped result objects, quoting nodes.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

// Structural traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_indexed_column_v;

    template<class T>
    using is_indexed_column = polyfill::bool_constant<is_indexed_column_v<T>>;

    template<class T>
    extern const bool is_columns_v;

    template<class T>
    using is_columns = polyfill::bool_constant<is_columns_v<T>>;

    template<class T>
    extern const bool is_struct_v;

    template<class T>
    using is_struct = polyfill::bool_constant<is_struct_v<T>>;

    template<class T>
    extern const bool is_object_node_v;

    template<class T>
    using is_object_node = polyfill::bool_constant<is_object_node_v<T>>;
}

// Quoting traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_column_pointer_v;

    template<class T>
    struct is_column_pointer : polyfill::bool_constant<is_column_pointer_v<T>> {};

    template<class T>
    extern const bool is_quoted_expression_v;

    template<class T>
    struct is_quoted_expression : polyfill::bool_constant<is_quoted_expression_v<T>> {};
}
