#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

namespace sqlite_orm::internal {
    /**
     *  DEFAULT constraint class.
     *  T is a value type.
     */
    template<class T>
    struct default_t {
        using value_type = T;

        value_type value;

        operator std::string() const {
            return "DEFAULT";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    constexpr internal::default_t<T> default_value(T t) {
        return {std::move(t)};
    }
}
