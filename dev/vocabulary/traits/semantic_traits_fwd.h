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
}
