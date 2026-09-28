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
    using is_select_expression = std::bool_constant<is_select_expression_v<T>>;

    /**
     *  Nodes that are or contain a DML statement expression.
     */
    template<class T, class SFINAE = void>
    constexpr bool is_raw_dml_expression_v = false;

    template<class T>
    using is_raw_dml_expression = std::bool_constant<is_raw_dml_expression_v<T>>;

    /**
     *  Nodes that are a DML statement expression bound to a mapped object:
     *  `insert(object)`, `replace(object)`, `update(object)`, `remove<O>(ids)`.
     *
     *  Note: bound to an object is not the same as carrying one - `remove<O>(ids)` names its
     *  object by primary key. Reaching for the object itself takes the additional check that
     *  `access_dml_object()` makes.
     */
    template<class T, class SFINAE = void>
    constexpr bool is_object_dml_expression_v = false;

    template<class T>
    using is_object_dml_expression = std::bool_constant<is_object_dml_expression_v<T>>;

    /**
     *  Nodes that are one of the built-in window functions: ROW_NUMBER(), RANK(), NTILE(N), LAG(expr), ...
     *
     *  Each is a function in its own right rather than a member of a grammar production, sharing only the role
     *  of being applied by an OVER clause - which is what the OVER node's consumers tell apart from an aggregate
     *  function applied by an OVER clause. Closed: defined once, next to the window functions.
     */
    template<class T>
    extern const bool is_builtin_window_function_v;

    template<class T>
    using is_builtin_window_function = std::bool_constant<is_builtin_window_function_v<T>>;
}
