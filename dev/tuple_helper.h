#pragma once

#include <tuple>  //  std::tuple, std::get, std::tuple_element
#include <type_traits>  //  std::false_type, std::true_type

#include "static_magic.h"

namespace sqlite_orm {

    //  got from here http://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
    namespace tuple_helper {
        /**
         *  HAS_TYPE type trait
         */
        template<typename T, typename Tuple>
        struct has_type;

        template<typename T>
        struct has_type<T, std::tuple<>> : std::false_type {};

        template<typename T, typename U, typename... Ts>
        struct has_type<T, std::tuple<U, Ts...>> : has_type<T, std::tuple<Ts...>> {};

        template<typename T, typename... Ts>
        struct has_type<T, std::tuple<T, Ts...>> : std::true_type {};

        template<typename T, typename Tuple>
        using tuple_contains_type = typename has_type<T, Tuple>::type;

        /**
         *  HAS_SOME_TYPE type trait
         */
        template<template<class> class TT, typename Tuple>
        struct has_some_type;

        template<template<class> class TT>
        struct has_some_type<TT, std::tuple<>> : std::false_type {};

        template<template<class> class TT, typename U, typename... Ts>
        struct has_some_type<TT, std::tuple<U, Ts...>> : has_some_type<TT, std::tuple<Ts...>> {};

        template<template<class> class TT, typename T, typename... Ts>
        struct has_some_type<TT, std::tuple<TT<T>, Ts...>> : std::true_type {};

        template<template<class> class TT, typename Tuple>
        using tuple_contains_some_type = typename has_some_type<TT, Tuple>::type;

        template<size_t N, class... Args>
        struct iterator_impl {

            template<class L>
            void operator()(const std::tuple<Args...>& tuple, const L& lambda, bool reverse = true) {
                if(reverse) {
                    lambda(std::get<N>(tuple));
                    iterator_impl<N - 1, Args...>()(tuple, lambda, reverse);
                } else {
                    iterator_impl<N - 1, Args...>()(tuple, lambda, reverse);
                    lambda(std::get<N>(tuple));
                }
            }

            template<class L>
            void operator()(const L& lambda) {
                iterator_impl<N - 1, Args...>()(lambda);
                lambda((const typename std::tuple_element<N - 1, std::tuple<Args...>>::type*)nullptr);
            }
        };

        template<class... Args>
        struct iterator_impl<0, Args...> {

            template<class L>
            void operator()(const std::tuple<Args...>& tuple, const L& lambda, bool /*reverse*/ = true) {
                lambda(std::get<0>(tuple));
            }

            template<class L>
            void operator()(const L& lambda) {
                lambda((const typename std::tuple_element<0, std::tuple<Args...>>::type*)nullptr);
            }
        };

        template<size_t N>
        struct iterator_impl<N> {

            template<class L>
            void operator()(const std::tuple<>&, const L&, bool /*reverse*/ = true) {
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
            void operator()(const L&) const {
                //..
            }
        };

        template<class H, class... Tail>
        struct iterator_impl2<H, Tail...> {

            template<class L>
            void operator()(const L& lambda) const {
                lambda((const H*)nullptr);
                iterator_impl2<Tail...>{}(lambda);
            }
        };

        template<class T>
        struct iterator;

        template<class... Args>
        struct iterator<std::tuple<Args...>> {

            template<class L>
            void operator()(const L& lambda) const {
                iterator_impl2<Args...>{}(lambda);
            }
        };

        template<size_t N, size_t I, class L, class R>
        void move_tuple_impl(L& lhs, R& rhs) {
            std::get<I>(lhs) = std::move(std::get<I>(rhs));
            internal::static_if<std::integral_constant<bool, N != I + 1>{}>([](auto& l, auto& r) {
                move_tuple_impl<N, I + 1>(l, r);
            })(lhs, rhs);
        }

        /**
         *  Accepts any number of arguments and evaluates `type` alias as T if all arguments are the same or void otherwise
         */
        template<class... Args>
        struct same_or_void {
            using type = void;
        };

        template<class A>
        struct same_or_void<A> {
            using type = A;
        };

        template<class A, class B>
        struct same_or_void<A, B> {
            using type = void;
        };

        template<class A>
        struct same_or_void<A, A> {
            using type = A;
        };

        template<class A, class... Args>
        struct same_or_void<A, A, Args...> {
            using type = typename same_or_void<A, Args...>::type;
        };

    }

    namespace internal {

        template<size_t N, class L, class R>
        void move_tuple(L& lhs, R& rhs) {
            using bool_type = std::integral_constant<bool, N != 0>;
            static_if<bool_type{}>([](auto& l, auto& r) {
                tuple_helper::move_tuple_impl<N, 0>(l, r);
            })(lhs, rhs);
        }

        template<class L, class... Args>
        void iterate_tuple(const std::tuple<Args...>& tuple, const L& lambda) {
            using tuple_type = std::tuple<Args...>;
            tuple_helper::iterator_impl<std::tuple_size<tuple_type>::value - 1, Args...>()(tuple, lambda, false);
        }

        template<class T, class L>
        void iterate_tuple(const L& lambda) {
            //            tuple_helper::iterator<T>()(lambda);
            tuple_helper::iterator<T>{}(lambda);
        }

        template<typename... input_t>
        using tuple_cat_t = decltype(std::tuple_cat(std::declval<input_t>()...));

        template<class... Args>
        struct conc_tuple {
            using type = tuple_cat_t<Args...>;
        };

        template<class T, template<class> class C>
        struct count_tuple;

        template<template<class> class C>
        struct count_tuple<std::tuple<>, C> {
            static constexpr const int value = 0;
        };

        template<class H, class... Args, template<class> class C>
        struct count_tuple<std::tuple<H, Args...>, C> {
            static constexpr const int value = C<H>::value + count_tuple<std::tuple<Args...>, C>::value;
        };
    }
}
