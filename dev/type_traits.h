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
        using match_if_not = std::enable_if_t<polyfill::negation<Op<Args...>>::value>;

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
        using object_type_t = typename T::object_type;
    }

    namespace internal {
#if __cplusplus >= 201703L  // use of C++17 or higher
        constexpr size_t _10_pow(size_t n) {
            if(n == 0) {
                return 1;
            } else {
                return 10 * _10_pow(n - 1);
            }
        }

        template<class... Chars, size_t... Is>
        constexpr size_t n_from_literal(std::index_sequence<Is...>, Chars... chars) {
            return (((chars - '0') * _10_pow(sizeof...(Is) - 1u - Is /*reversed index sequence*/)) + ...);
        }

        template<unsigned int N>
        struct positional_ordinal : std::integral_constant<unsigned int, N> {};
#endif
    }
}
