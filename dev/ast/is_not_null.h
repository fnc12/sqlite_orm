#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../tags.h"
#include "../functional/config.h"

namespace sqlite_orm::internal {
    /**
     *  IS NOT NULL operator object.
     */
    template<class T>
    struct is_not_null_t : condition_t, negatable_t {
        using argument_type = T;
        using self = is_not_null_t<argument_type>;

        argument_type argument;

        is_not_null_t(argument_type argument_) : argument(std::move(argument_)) {}
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  IS NOT NULL operator.
     */
    template<class T>
    internal::is_not_null_t<T> is_not_null(T t) {
        return {std::move(t)};
    }
}
