#pragma once

#include <type_traits>  //  std::enable_if, std::is_same, std::is_empty
#if __cpp_lib_unwrap_ref >= 201811L
#include <utility>  //  std::reference_wrapper
#else
#include <functional>  //  std::reference_wrapper
#endif

#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    // C++ generic traits used throughout the library
    namespace internal {
        template<class T, class... Types>
        using is_any_of = polyfill::disjunction<std::is_same<T, Types>...>;

        template<class T>
        struct value_unref_type : polyfill::remove_cvref<T> {};

        template<class T>
        struct value_unref_type<std::reference_wrapper<T>> : std::remove_const<T> {};

        template<class T>
        using value_unref_type_t = typename value_unref_type<T>::type;

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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<auto a>
        using auto_type_t = typename decltype(a)::type;
#endif

        template<typename T>
        using field_type_t = typename T::field_type;

        template<typename T>
        using constraints_type_t = typename T::constraints_type;

        template<typename T>
        using columns_tuple_t = typename T::columns_tuple;

        template<typename T>
        using object_type_t = typename T::object_type;

        template<typename T>
        using elements_type_t = typename T::elements_type;

        template<typename T>
        using target_type_t = typename T::target_type;

        template<typename T>
        using left_type_t = typename T::left_type;

        template<typename T>
        using right_type_t = typename T::right_type;

        template<typename T>
        using on_type_t = typename T::on_type;

        template<typename T>
        using expression_type_t = typename T::expression_type;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<class T>
        using udf_type_t = typename T::udf_type;

        template<auto a>
        using auto_udf_type_t = typename decltype(a)::udf_type;
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        template<typename T>
        concept stateless = std::is_empty_v<T>;
#endif
    }

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept orm_names_type = requires { typename T::type; };
#endif
}
