#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct excluded_t {
        using expression_type = T;

        expression_type expression;
    };

    template<class T>
    constexpr bool is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, excluded_t>::value>> =
        true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::excluded_t<T> excluded(T expression) {
        return {std::move(expression)};
    }
}
