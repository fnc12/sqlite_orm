#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct check_t {
        using expression_type = T;

        expression_type expression;
    };

    template<class T>
    constexpr bool is_check_v = polyfill::is_specialization_of_v<T, check_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    constexpr internal::check_t<T> check(T t) {
        return {std::move(t)};
    }
}
