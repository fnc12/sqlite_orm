#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same, std::disjunction, std::negation, std::is_empty, std::is_aggregate, std::is_function, std::common_type
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  //  std::same_as
#endif
#endif

#include "cxx_polyfill/type_traits.h"

// generic type traits
namespace sqlite_orm::internal {
    template<class T>
    using is_eval_order_guaranteed = std::is_aggregate<T>;

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<typename T>
    concept stateless = std::is_empty_v<T>;
#endif
}

// type algorithms over packs
namespace sqlite_orm::internal {
    template<class T, class... Types>
    using is_any_of = std::disjunction<std::is_same<T, Types>...>;

    template<class Pack>
    struct common_type_of;

    template<template<class...> class Pack, class... Types>
    struct common_type_of<Pack<Types...>> : std::common_type<Types...> {};

    /**
     *  Accepts a pack of types and defines a nested `type` typename to a common type if possible, otherwise nonexistent.
     *
     *  @note: SFINAE friendly like `std::common_type`.
     */
    template<class Pack>
    using common_type_of_t = typename common_type_of<Pack>::type;

    /**
     *  Accepts any number of arguments and evaluates a nested `type` typename as `T` if all arguments are the same, otherwise `void`.
     */
    template<class... Args>
    struct same_or_void {
        using type = void;
    };

    template<class... Args>
    using same_or_void_t = typename same_or_void<Args...>::type;

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class A, std::same_as<A>... Rest>
    struct same_or_void<A, Rest...> {
        using type = A;
    };
#else
    template<class A>
    struct same_or_void<A> {
        using type = A;
    };

    template<class A>
    struct same_or_void<A, A> {
        using type = A;
    };

    template<class A, class... Args>
    struct same_or_void<A, A, Args...> : same_or_void<A, Args...> {};
#endif
}

// SFINAE helpers
namespace sqlite_orm::internal {
    // enable_if for types
    template<template<typename...> class Op, class... Args>
    using match_if = std::enable_if_t<Op<Args...>::value>;

    template<template<typename...> class Op, class... Args>
    using match_if_not = std::enable_if_t<std::negation<Op<Args...>>::value>;

    template<class T, template<typename...> class Primary>
    using match_specialization_of = std::enable_if_t<polyfill::is_specialization_of<T, Primary>::value>;

    // enable_if for functions
    template<template<typename...> class Op, class... Args>
    using satisfies = std::enable_if_t<Op<Args...>::value, bool>;

    template<template<typename...> class Op, class... Args>
    using satisfies_not = std::enable_if_t<std::negation<Op<Args...>>::value, bool>;

    template<class T, template<typename...> class Primary>
    using satisfies_specialization_of = std::enable_if_t<polyfill::is_specialization_of<T, Primary>::value, bool>;
}

// type name projectors
namespace sqlite_orm::internal {
    template<typename T>
    using type_t = typename T::type;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<auto a>
    using auto_type_t = typename decltype(a)::type;
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept orm_names_type = requires { typename T::type; };

    /** @short Specifies that a type is a function signature (i.e. a function in the C++ type system).
     */
    template<class Sig>
    concept orm_function_sig = std::is_function_v<Sig>;
#endif
}
