#pragma once

#include <tuple>  //  std::tuple, std::get, std::tuple_element, std::tuple_size
#include <type_traits>  //  std::is_same
#include <utility>  //  std::forward

#include "../cxx_polyfill.h"

namespace sqlite_orm {

    namespace tuple_helper {

        template<class T, class Tuple>
        struct tuple_contains_type;
        template<class T, class... Args>
        struct tuple_contains_type<T, std::tuple<Args...>> : polyfill::disjunction<std::is_same<T, Args>...> {};

        template<template<class> class TT, class Tuple>
        struct tuple_contains_some_type;
        template<template<class> class TT, class... Args>
        struct tuple_contains_some_type<TT, std::tuple<Args...>>
            : polyfill::disjunction<polyfill::is_specialization_of<Args, TT>...> {};

#if !defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) || !defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<size_t N, class... Args>
        struct iterator_impl {

            template<class L>
            void operator()(const std::tuple<Args...>& tuple, L& lambda, bool reverse = true) {
                if(reverse) {
                    lambda(std::get<N>(tuple));
                    iterator_impl<N - 1, Args...>()(tuple, lambda, reverse);
                } else {
                    iterator_impl<N - 1, Args...>()(tuple, lambda, reverse);
                    lambda(std::get<N>(tuple));
                }
            }

            template<class L>
            void operator()(L& lambda) {
                iterator_impl<N - 1, Args...>()(lambda);
                lambda((const std::tuple_element_t<N - 1, std::tuple<Args...>>*)nullptr);
            }
        };

        template<class... Args>
        struct iterator_impl<0, Args...> {

            template<class L>
            void operator()(const std::tuple<Args...>& tuple, L& lambda, bool /*reverse*/ = true) {
                lambda(std::get<0>(tuple));
            }

            template<class L>
            void operator()(L& lambda) {
                lambda((const std::tuple_element_t<0, std::tuple<Args...>>*)nullptr);
            }
        };

        template<size_t N>
        struct iterator_impl<N> {

            template<class L>
            void operator()(const std::tuple<>&, L&, bool /*reverse*/ = true) {
                //..
            }

            template<class L>
            void operator()(const L&) {
                //..
            }
        };
#endif

#ifndef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        template<class... Args>
        struct iterator_impl2;

        template<>
        struct iterator_impl2<> {

            template<class L>
            void operator()(L&) const {
                //..
            }
        };

        template<class H, class... Tail>
        struct iterator_impl2<H, Tail...> {

            template<class L>
            void operator()(L& lambda) const {
                lambda((const H*)nullptr);
                iterator_impl2<Tail...>{}(lambda);
            }
        };

        template<class T>
        struct iterator;

        template<class... Args>
        struct iterator<std::tuple<Args...>> {

            template<class L>
            void operator()(L& lambda) const {
                iterator_impl2<Args...>{}(lambda);
            }
        };
#endif
    }

    namespace internal {

        //  got it form here https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
        template<class Function, class FunctionPointer, class Tuple, size_t... I>
        auto call_impl(Function& f, FunctionPointer functionPointer, Tuple t, std::index_sequence<I...>) {
            return (f.*functionPointer)(std::get<I>(move(t))...);
        }

        template<class Function, class FunctionPointer, class Tuple>
        auto call(Function& f, FunctionPointer functionPointer, Tuple t) {
            constexpr size_t size = std::tuple_size<Tuple>::value;
            return call_impl(f, functionPointer, move(t), std::make_index_sequence<size>{});
        }

        template<class Function, class Tuple>
        auto call(Function& f, Tuple t) {
            return call(f, &Function::operator(), move(t));
        }

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<bool reversed = false, class Tpl, class L, size_t... Idx>
        void iterate_tuple(const Tpl& tpl, L&& lambda, std::index_sequence<Idx...>) {
            if constexpr(reversed) {
                (lambda(std::get<sizeof...(Idx) - 1u - Idx>(tpl)), ...);
            } else {
                (lambda(std::get<Idx>(tpl)), ...);
            }
        }
        template<bool reversed = false, class Tpl, class L>
        void iterate_tuple(const Tpl& tpl, L&& lambda) {
            iterate_tuple<reversed>(tpl, std::forward<L>(lambda), std::make_index_sequence<std::tuple_size_v<Tpl>>{});
        }
#else
        template<class L, class... Args>
        void iterate_tuple(const std::tuple<Args...>& tuple, L&& lambda) {
            using tuple_type = std::tuple<Args...>;
            tuple_helper::iterator_impl<std::tuple_size<tuple_type>::value - 1, Args...>()(tuple, lambda, false);
        }
#endif

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        template<class Tpl, class L, size_t... Idx>
        void iterate_tuple(L&& lambda, std::index_sequence<Idx...>) {
            (lambda((std::tuple_element_t<Idx, Tpl>*)nullptr), ...);
        }
        template<class Tpl, class L>
        void iterate_tuple(L&& lambda) {
            iterate_tuple<Tpl>(std::forward<L>(lambda), std::make_index_sequence<std::tuple_size_v<Tpl>>{});
        }
#else
        template<class Tpl, class L>
        void iterate_tuple(L&& lambda) {
            tuple_helper::iterator<Tpl>{}(lambda);
        }
#endif
    }
}
