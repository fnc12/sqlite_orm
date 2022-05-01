#pragma once

#include <tuple>  //  std::tuple, std::get, std::tuple_element, std::tuple_size
#include <type_traits>  //  std::is_same

#include "../cxx_polyfill.h"
#include "../static_magic.h"

namespace sqlite_orm {

    //  got from here http://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
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

        template<size_t N, size_t I, class L, class R>
        void move_tuple_impl(L& lhs, R& rhs) {
            std::get<I>(lhs) = std::move(std::get<I>(rhs));
            internal::static_if<N != I + 1>([](auto& l, auto& r) {
                move_tuple_impl<N, I + 1>(l, r);
            })(lhs, rhs);
        }

        template<size_t N, class L, class R>
        void move_tuple(L& lhs, R& rhs) {
            static_if<N != 0>([](auto& l, auto& r) {
                move_tuple_impl<N, 0>(l, r);
            })(lhs, rhs);
        }

        template<class L, class... Args>
        void iterate_tuple(const std::tuple<Args...>& tuple, L&& lambda) {
            using tuple_type = std::tuple<Args...>;
            tuple_helper::iterator_impl<std::tuple_size<tuple_type>::value - 1, Args...>()(tuple, lambda, false);
        }

        template<class T, class L>
        void iterate_tuple(L&& lambda) {
            tuple_helper::iterator<T>{}(lambda);
        }
    }
}
