#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same, std::is_empty, std::is_aggregate
#if __cpp_lib_unwrap_ref >= 201811L
#include <utility>  //  std::reference_wrapper
#else
#include <functional>  //  std::reference_wrapper
#endif
#endif

#include "functional/cxx_type_traits_polyfill.h"

// C++ generic traits used throughout the library
namespace sqlite_orm::internal {
    template<class T, class... Types>
    using is_any_of = std::disjunction<std::is_same<T, Types>...>;

    template<class T>
    struct value_unref_type : polyfill::remove_cvref<T> {};

    template<class T>
    struct value_unref_type<std::reference_wrapper<T>> : std::remove_const<T> {};

    template<class T>
    using value_unref_type_t = typename value_unref_type<T>::type;

    template<class T>
    using is_eval_order_garanteed =
#if __cpp_lib_is_aggregate >= 201703L
        std::is_aggregate<T>;
#else
        std::is_pod<T>;
#endif

    // enable_if for types
    template<template<typename...> class Op, class... Args>
    using match_if = std::enable_if_t<Op<Args...>::value>;

    // enable_if for types
    template<template<typename...> class Op, class... Args>
    using match_if_not = std::enable_if_t<std::negation<Op<Args...>>::value>;

    // enable_if for types
    template<class T, template<typename...> class Primary>
    using match_specialization_of = std::enable_if_t<polyfill::is_specialization_of<T, Primary>::value>;

    // enable_if for functions
    template<template<typename...> class Op, class... Args>
    using satisfies = std::enable_if_t<Op<Args...>::value, bool>;

    // enable_if for functions
    template<template<typename...> class Op, class... Args>
    using satisfies_not = std::enable_if_t<std::negation<Op<Args...>>::value, bool>;

    // enable_if for functions
    template<class T, template<typename...> class Primary>
    using satisfies_is_specialization_of = std::enable_if_t<polyfill::is_specialization_of<T, Primary>::value, bool>;
}

// type name template alias projectors for syntactic sugar
namespace sqlite_orm::internal {
    template<typename T>
    using type_t = typename T::type;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<auto a>
    using auto_type_t = typename decltype(a)::type;
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<typename T>
    concept stateless = std::is_empty_v<T>;
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept orm_names_type = requires { typename T::type; };
#endif
}
