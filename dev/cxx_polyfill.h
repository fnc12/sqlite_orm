#pragma once

#include <type_traits>

#include "start_macros.h"

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
#if __cplusplus < 201703L  // before C++17
            template<bool v>
            using bool_constant = std::integral_constant<bool, v>;

            template<typename...>
            struct conjunction : std::true_type {};
            template<typename B1>
            struct conjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct conjunction<B1, Bn...> : std::conditional_t<B1::value, conjunction<Bn...>, B1> {};
            template<typename... Bs>
            constexpr bool conjunction_v = conjunction<Bs...>::value;

            template<typename...>
            struct disjunction : std::false_type {};
            template<typename B1>
            struct disjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct disjunction<B1, Bn...> : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};
            template<typename... Bs>
            constexpr bool disjunction_v = disjunction<Bs...>::value;

            template<class B>
            struct negation : bool_constant<!bool(B::value)> {};

            template<class...>
            using void_t = void;
#else
            using std::bool_constant;
            using std::conjunction, std::conjunction_v;
            using std::disjunction, std::disjunction_v;
            using std::negation;
            using std::void_t;
#endif

#if __cplusplus < 202002L  // before C++20
            template<class T>
            struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};

            template<class T>
            using remove_cvref_t = typename remove_cvref<T>::type;
#else
            using std::remove_cvref;
            using std::remove_cvref_t;
#endif

#if __cplusplus < 202312L  // before C++23
            template<typename Type, template<typename...> class Primary>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v = false;

            template<template<typename...> class Primary, class... Types>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v<Primary<Types...>, Primary> = true;

            template<typename Type, template<typename...> class Primary>
            struct is_specialization_of : bool_constant<is_specialization_of_v<Type, Primary>> {};

            template<typename... T>
            using is_specialization_of_t = typename is_specialization_of<T...>::type;
#else
            using std::is_specialization_of, std::is_specialization_of_t, std::is_specialization_of_v;
#endif

            template<typename...>
            SQLITE_ORM_INLINE_VAR constexpr bool always_false_v = false;

            template<size_t I>
            using index_constant = std::integral_constant<size_t, I>;
        }
    }
}
