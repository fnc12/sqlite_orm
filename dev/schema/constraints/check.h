#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

namespace sqlite_orm::internal {
    template<class T>
    struct check_t {
        using expression_type = T;

        expression_type expression;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    constexpr internal::check_t<T> check(T t) {
        return {std::move(t)};
    }
}
