#pragma once

/** @file
 *  Purpose: declare the traits classifying the DSL nodes that have no SQL grammar counterpart.
 *
 *  Intention: the structural axis of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md`
 *  holds; the traits are defined in the corresponding header files of the nodes.
 */

#include "../../functional/cxx_polyfill/type_traits.h"

// Structural traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_indexed_column_v;

    template<class T>
    using is_indexed_column = std::bool_constant<is_indexed_column_v<T>>;

    template<class T>
    extern const bool is_columns_v;

    template<class T>
    using is_columns = std::bool_constant<is_columns_v<T>>;

    template<class T>
    extern const bool is_struct_v;

    template<class T>
    using is_struct = std::bool_constant<is_struct_v<T>>;

    template<class T>
    extern const bool is_object_node_v;

    template<class T>
    using is_object_node = std::bool_constant<is_object_node_v<T>>;

    /**
     *  Nodes turning the C++ result of a result column into an optional; no SQL counterpart.
     */
    template<class T>
    extern const bool is_as_optional_v;

    template<class T>
    using is_as_optional = std::bool_constant<is_as_optional_v<T>>;
}

// Quoting traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_column_pointer_v;

    template<class T>
    using is_column_pointer = std::bool_constant<is_column_pointer_v<T>>;

    template<class T>
    extern const bool is_quoted_expression_v;

    template<class T>
    using is_quoted_expression = std::bool_constant<is_quoted_expression_v<T>>;

    /**
     *  Nodes protecting an otherwise bindable value, so that it is serialized as a literal.
     */
    template<class T>
    extern const bool is_literal_v;

    template<class T>
    using is_literal = std::bool_constant<is_literal_v<T>>;

    /**
     *  Nodes lifting a struct mapped as a table or view into the DSL, enabling `->*`: `c<Object>()`.
     */
    template<class T>
    extern const bool is_table_reference_v;

    template<class T>
    using is_table_reference = std::bool_constant<is_table_reference_v<T>>;
}
