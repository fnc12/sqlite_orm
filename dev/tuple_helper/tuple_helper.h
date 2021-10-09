#pragma once

#include <tuple>  //  std::tuple, std::get, std::tuple_element, std::tuple_size
#include <type_traits>  //  std::false_type, std::true_type

#include "static_magic.h"
#include "common_traits.h"
#include "valuebased_metaprogramming.h"

namespace sqlite_orm {
    namespace internal {
        /**
         *  HAS_TYPE value-based metafunction
         */
        template<typename T, typename... Args>
        constexpr bool has_type(sqlite_orm::internal::tuple_t<Args...>) noexcept {
            bool result[] = {(std::is_same<T, Args>::value)..., false};
            for(bool value: result) {
                if(value) {
                    return true;
                }
            }
            return false;
        }

        template<typename T, typename Tuple>
        using tuple_contains_type =
            std::integral_constant<bool, has_type<T>(typename sqlite_orm::internal::valuebased_tuple<Tuple>::type{})>;

        /**
         *  HAS_SOME_TYPE value-based metafunction
         */
        template<template<class...> class TT, typename... Args>
        constexpr bool has_some_type(sqlite_orm::internal::tuple_t<Args...>) noexcept {
            bool result[] = {(sqlite_orm::internal::is_template_matches_type<TT, Args>::value)..., false};
            for(bool value: result) {
                if(value) {
                    return true;
                }
            }
            return false;
        }

        template<template<class...> class TT, typename Tuple>
        using tuple_contains_some_type =
            std::integral_constant<bool,
                                   has_some_type<TT>(typename sqlite_orm::internal::valuebased_tuple<Tuple>::type{})>;

        //  got it form here https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
        template<class Function, class FunctionPointer, class Tuple, size_t... I>
        auto call_impl(Function& f, FunctionPointer functionPointer, Tuple t, std::index_sequence<I...>) {
            return (f.*functionPointer)(std::get<I>(t)...);
        }

        template<class Function, class FunctionPointer, class Tuple>
        auto call(Function& f, FunctionPointer functionPointer, Tuple t) {
            static constexpr auto size = std::tuple_size<Tuple>::value;
            return call_impl(f, functionPointer, move(t), std::make_index_sequence<size>{});
        }

        template<class Function, class Tuple>
        auto call(Function& f, Tuple t) {
            return call(f, &Function::operator(), move(t));
        }

        // inspired by https://github.com/boostorg/pfr/blob/master/include/boost/pfr/detail/for_each_field_impl.hpp
        template<class T, class L, std::size_t... I>
        void iterate_tuple_impl(const T& t, const L& l, std::index_sequence<I...>) {
            const int v[] = {0, (l(std::get<I>(t)), 0)...};
            (void)v;
        }

        template<class T, class L, std::size_t... I>
        void iterate_tuple_impl(const L& l, std::index_sequence<I...>) {
            const int v[] = {0, (l((const typename std::tuple_element<I, T>::type*)nullptr), 0)...};
            (void)v;
        }

        template<class L, class... Args>
        void iterate_tuple(const std::tuple<Args...>& tuple, const L& lambda) {
            using tuple_type = std::tuple<Args...>;
            static constexpr auto size = std::tuple_size<tuple_type>::value;

            iterate_tuple_impl(tuple, lambda, std::make_index_sequence<size>{});
        }

        template<class T, class L>
        void iterate_tuple(const L& lambda) {
            static constexpr auto size = std::tuple_size<T>::value;

            iterate_tuple_impl<T>(lambda, std::make_index_sequence<size>{});
        }

        template<typename... input_t>
        using tuple_cat_t = decltype(std::tuple_cat(std::declval<input_t>()...));

        template<class... Args>
        struct conc_tuple {
            using type = tuple_cat_t<Args...>;
        };
    }
}
