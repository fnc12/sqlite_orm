#pragma once

/** @file Cross-cutting role/capability traits in purview of the sqlite_orm EDSL.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    /**
     *  Nodes that are or contain a main select statement expression.
     *  E.g. WITH + SELECT both "select expressions"
     */
    template<class T, class SFINAE = void>
    constexpr bool is_select_expression_v = false;

    template<class T>
    using is_select_expression = polyfill::bool_constant<is_select_expression_v<T>>;

    /**
     *  Nodes that are or contain a DML statement expression.
     */
    template<class T, class SFINAE = void>
    constexpr bool is_raw_dml_expression_v = false;

    template<class T>
    using is_raw_dml_expression = polyfill::bool_constant<is_raw_dml_expression_v<T>>;

    /**
     *  Nodes that are a DML statement expression bound to a mapped object:
     *  `insert(object)`, `replace(object)`, `update(object)`, `remove<O>(ids)`.
     */
    template<class T, class SFINAE = void>
    constexpr bool is_object_dml_expression_v = false;

    template<class T>
    using is_object_dml_expression = polyfill::bool_constant<is_object_dml_expression_v<T>>;
}
