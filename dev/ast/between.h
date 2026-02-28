#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif
#include "tags.h"

namespace sqlite_orm::internal {
    /**
     *  BETWEEN operator object.
     */
    template<class A, class T>
    struct between_t : condition_t, negatable_t {
        using expression_type = A;
        using lower_type = T;
        using upper_type = T;

        expression_type expression;
        lower_type lower;
        upper_type upper;

        between_t(expression_type expression_, lower_type lower_, upper_type upper_) :
            expression(std::move(expression_)), lower(std::move(lower_)), upper(std::move(upper_)) {}
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  X BETWEEN Y AND Z
     *  Example: storage.select(between(&User::id, 10, 20))
     */
    template<class A, class T>
    internal::between_t<A, T> between(A expression, T lower, T upper) {
        return {std::move(expression), std::move(lower), std::move(upper)};
    }
}