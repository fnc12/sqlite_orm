#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/node_algorithms.h"  // unwrap_expression
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T, class X>
    struct match_with_table_t {
        using mapped_type = T;
        using argument_type = X;

        argument_type argument;
    };

    template<class T>
    constexpr bool is_match_with_table_v = polyfill::is_specialization_of_v<T, match_with_table_t>;

    /*
     *  Alternative equality comparison where the left side is always a field.
     */
    template<class Field, class X>
    struct match_t {
        using field_type = Field;
        using argument_type = X;

        field_type field;
        argument_type argument;
    };

    template<class T>
    constexpr bool is_match_v = polyfill::is_specialization_of_v<T, match_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class T, class X>
    [[deprecated(
        "Use the `match` function accepting the hidden FTS5 'any' field or a field of your FTS table instead")]]
    constexpr internal::match_with_table_t<T, internal::unwrap_expression_t<X>> match(X argument) {
        return {internal::unwrap_expression(std::move(argument))};
    }

    template<class CP, class X>
    constexpr internal::match_t<internal::unwrap_expression_t<CP>, internal::unwrap_expression_t<X>> match(CP field,
                                                                                                           X argument) {
        return {internal::unwrap_expression(std::move(field)), internal::unwrap_expression(std::move(argument))};
    }

    template<class O, class F, class X>
    constexpr internal::match_t<F O::*, internal::unwrap_expression_t<X>> match(F O::* field, X argument) {
        return {field, internal::unwrap_expression(std::move(argument))};
    }
}
