#pragma once

#include <type_traits>

#include "start_macros.h"
#include "cxx_polyfill.h"

namespace sqlite_orm {
    // C++ generic traits used throughout the library
    namespace internal {
        template<class T, class... Types>
        using is_any_of = polyfill::disjunction<std::is_same<T, Types>...>;

        template<class T, class... Types>
        SQLITE_ORM_INLINE_VAR constexpr bool is_any_of_v = polyfill::disjunction_v<std::is_same<T, Types>...>;

        // enable_if for types
        template<template<typename...> class Op, class... Args>
        using match_if = std::enable_if_t<Op<Args...>::value>;

        // enable_if for types
        template<template<typename...> class Op, class... Args>
        using match_if_not = std::enable_if_t<polyfill::negation_v<Op<Args...>>>;

        // enable_if for types
        template<class T, template<typename...> class Primary>
        using match_specialization_of = std::enable_if_t<polyfill::is_specialization_of_v<T, Primary>>;

        // enable_if for functions
        template<template<typename...> class Op, class... Args>
        using satisfies = std::enable_if_t<Op<Args...>::value, bool>;

        // enable_if for functions
        template<template<typename...> class Op, class... Args>
        using satisfies_not = std::enable_if_t<polyfill::negation<Op<Args...>>::value, bool>;

        // enable_if for functions
        template<class T, template<typename...> class Primary>
        using satisfies_is_specialization_of = std::enable_if_t<polyfill::is_specialization_of_v<T, Primary>, bool>;
    }

    // type name template aliases for syntactic sugar
    namespace internal {
        template<typename T>
        using type_t = typename T::type;

        template<typename T>
        using field_type_t = typename T::field_type;

        template<typename T>
        using object_type_t = typename T::object_type;

        template<typename T>
        using table_type_t = typename T::table_type;

        template<typename S>
        using storage_object_type_t = typename S::table_type::object_type;
    }
}
