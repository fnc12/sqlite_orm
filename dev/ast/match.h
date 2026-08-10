#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

namespace sqlite_orm::internal {
    template<class T, class X>
    struct match_with_table_t {
        using mapped_type = T;
        using argument_type = X;

        argument_type argument;
    };

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
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class T, class X>
    [[deprecated(
        "Use the `match` function accepting the hidden FTS5 'any' field or a field of your FTS table instead")]]
    constexpr internal::match_with_table_t<T, X> match(X argument) {
        return {std::move(argument)};
    }

    template<class CP, class X>
    constexpr internal::match_t<CP, X> match(CP field, X argument) {
        return {std::move(field), std::move(argument)};
    }

    template<class O, class F, class X>
    constexpr internal::match_t<F O::*, X> match(F O::* field, X argument) {
        return {field, std::move(argument)};
    }
}
