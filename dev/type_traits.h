#pragma once

#include <type_traits>

#include "cxx_polyfill.h"

namespace sqlite_orm {
    // C++ generic traits used throughout the library
    namespace internal {
        // enable_if for types
        template<template<typename...> class Op, class... Args>
        using match_if = std::enable_if_t<Op<Args...>::value>;

        // enable_if for types
        template<template<typename...> class Op, class... Args>
        using match_if_not = std::enable_if_t<std::negation<Op<Args...>>::value>;

        // enable_if for types
        template<class T, template<typename...> class Primary>
        using match_specialization_of = std::enable_if_t<polyfill::is_specialization_of_v<T, Primary>>;

        // enable_if for functions
        template<template<typename...> class Op, class... Args>
        using satisfies = std::enable_if_t<Op<Args...>::value, bool>;

        // enable_if for functions
        template<template<typename...> class Op, class... Args>
        using satisfies_not = std::enable_if_t<std::negation<Op<Args...>>::value, bool>;

        // enable_if for functions
        template<class T, template<typename...> class Primary>
        using satisfies_is_specialization_of = std::enable_if_t<polyfill::is_specialization_of_v<T, Primary>, bool>;
    }

    // type traits not quite polyfill, however used throughout the program
    namespace internal {}

    // type name template aliases for syntactic sugar
    namespace internal {
        template<typename T>
        using object_type_t = typename T::object_type;
    }
}
