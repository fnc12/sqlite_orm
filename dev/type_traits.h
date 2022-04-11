#pragma once

#include <type_traits>
#include <tuple>

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
        using value_type_t = typename T::value_type;

        template<typename T>
        using field_type_t = typename T::field_type;

        template<typename T>
        using object_type_t = typename T::object_type;

        template<typename T>
        using cte_label_type_t = typename T::cte_label_type;

        template<typename T>
        using cte_object_type_t = typename T::cte_object_type;

        template<typename T>
        using table_type_t = typename T::table_type;

        template<typename S>
        using storage_object_type_t = typename S::table_type::object_type;

        template<typename S>
        using storage_cte_mapper_type_t = typename S::table_type::cte_mapper_type;

        template<typename S>
        using storage_cte_label_type_t = typename S::table_type::cte_label_type;

        template<typename T>
        using expression_type_t = typename T::expression_type;

        template<class As>
        using alias_type_t = typename As::alias_type;

        // T::alias_type or nonesuch
        template<class T>
        using alias_type_or_none = polyfill::detected<alias_type_t, T>;
    }

    namespace internal {
        template<unsigned int N>
        using nth_constant = std::integral_constant<unsigned int, N>;

        template<typename... Fs>
        using fields_t = std::tuple<Fs...>;

#if __cplusplus >= 201703L  // use of C++17 or higher
        // cudos to OznOg https://stackoverflow.com/a/64606884/279251
        template<class X, class Tuple>
        struct tuple_index_of;

        template<class X, class... T>
        struct tuple_index_of<X, std::tuple<T...>> {
            template<size_t... idx>
            static constexpr ptrdiff_t find_idx(std::index_sequence<idx...>) {
                return std::max({static_cast<ptrdiff_t>(std::is_same<X, T>::value ? idx : -1)...});
            }

            static constexpr ptrdiff_t value = find_idx(std::index_sequence_for<T...>{});
        };
        template<class X, class Tuple>
        inline constexpr ptrdiff_t tuple_index_of_v = tuple_index_of<X, Tuple>::value;
#endif

        template<typename T>
        struct tuplify {
            using type = std::tuple<T>;
        };
        template<typename... Ts>
        struct tuplify<std::tuple<Ts...>> {
            using type = std::tuple<Ts...>;
        };

        template<typename T>
        using tuplify_t = typename tuplify<T>::type;
    }
}
