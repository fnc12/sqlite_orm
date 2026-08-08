#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    struct unique_base {
        operator std::string() const {
            return "UNIQUE";
        }
    };

    /**
     *  UNIQUE constraint class.
     */
    template<class... Args>
    struct unique_t : unique_base {
        using columns_tuple = std::tuple<Args...>;

        columns_tuple columns;

        constexpr unique_t(columns_tuple columns_) : columns(std::move(columns_)) {}
    };

    template<class T>
    constexpr bool is_unique_v = polyfill::is_specialization_of_v<T, unique_t>;

    template<class T>
    struct is_unique : polyfill::bool_constant<is_unique_v<T>> {};
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  UNIQUE table constraint factory function.
     */
    template<class... Args>
    constexpr internal::unique_t<Args...> unique(Args... args) {
        return {{std::forward<Args>(args)...}};
    }

    /**
     *  UNIQUE column constraint factory function.
     */
    constexpr internal::unique_t<> unique() {
        return {{}};
    }
}
