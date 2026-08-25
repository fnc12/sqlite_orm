#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../tags.h"
#include "../vocabulary/node_algorithms.h"

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
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  IS NULL operator.
     */
    template<class T>
    internal::is_null_t<T> is_null(T t) {
        static_assert(internal::is_operand_or_bindable<T>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(t)};
    }
}