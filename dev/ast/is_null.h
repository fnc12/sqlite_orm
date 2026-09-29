#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tags.h"
#include "../vocabulary/node_algorithms.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  IS NULL operator object.
     */
    template<class T>
    struct is_null_t : condition_t, negatable_t {
        using argument_type = T;

        argument_type argument;

        is_null_t(argument_type argument_) : argument(std::move(argument_)) {}
    };

    template<class T>
    constexpr bool is_is_null_v = polyfill::is_specialization_of_v<T, is_null_t>;

    /**
     *  IS NOT NULL operator object.
     */
    template<class T>
    struct is_not_null_t : condition_t, negatable_t {
        using argument_type = T;

        argument_type argument;

        is_not_null_t(argument_type argument_) : argument(std::move(argument_)) {}
    };

    template<class T>
    constexpr bool is_is_not_null_v = polyfill::is_specialization_of_v<T, is_not_null_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  IS NULL operator.
     */
    template<class T>
    internal::is_null_t<T> is_null(T expression) {
        static_assert(internal::is_operand_or_bindable<T>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(expression)};
    }

    /**
     *  IS NOT NULL operator.
     */
    template<class T>
    internal::is_not_null_t<T> is_not_null(T expression) {
        static_assert(internal::is_operand_or_bindable<T>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(expression)};
    }
}
