#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <utility>  // std::move
#endif  //  SQLITE_ORM_IMPORT_STD_MODULE

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  CAST holder.
     *  T is a type to cast to
     *  E is an expression type
     *  Example: cast<std::string>(&User::id)
     */
    template<class T, class E>
    struct cast_t {
        using to_type = T;
        using expression_type = E;

        expression_type expression;
    };

    template<class T>
    constexpr bool is_cast_v = polyfill::is_specialization_of_v<T, cast_t>;

    template<class T>
    constexpr bool is_operator_argument_v<T, std::enable_if_t<is_cast_v<T>>> = true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  CAST(X AS type).
     *  Example: cast<std::string>(&User::id)
     */
    template<class T, class E>
    internal::cast_t<T, E> cast(E e) {
        return {std::move(e)};
    }
}
