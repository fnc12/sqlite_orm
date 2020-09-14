#pragma once

#include <tuple>  //  std::tuple, std::get
#include <type_traits>  //  std::false_type, std::true_type

#include "static_magic.h"

namespace sqlite_orm {

    //  got from here http://stackoverflow.com/questions/25958259/how-do-i-find-out-if-a-tuple-contains-a-type
    namespace tuple_helper {

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

        template<size_t N, class... Args>
        struct iterator {

            template<class L>
            void operator()(const std::tuple<Args...> &t, const L &l, bool reverse = true) {
                if(reverse) {
                    l(std::get<N>(t));
                    iterator<N - 1, Args...>()(t, l, reverse);
                } else {
                    iterator<N - 1, Args...>()(t, l, reverse);
                    l(std::get<N>(t));
                }
            }
        };

        template<class... Args>
        struct iterator<0, Args...> {

            template<class L>
            void operator()(const std::tuple<Args...> &t, const L &l, bool /*reverse*/ = true) {
                l(std::get<0>(t));
            }
        };

        template<size_t N>
        struct iterator<N> {

            template<class L>
            void operator()(const std::tuple<> &, const L &, bool /*reverse*/ = true) {
                //..
            }
        };

        template<size_t N, size_t I, class L, class R>
        void move_tuple_impl(L &lhs, R &rhs) {
            std::get<I>(lhs) = std::move(std::get<I>(rhs));
            internal::static_if<std::integral_constant<bool, N != I + 1>{}>([](auto &l, auto &r) {
                move_tuple_impl<N, I + 1>(l, r);
            })(lhs, rhs);
        }
    }

    namespace internal {

        template<size_t N, class L, class R>
        void move_tuple(L &lhs, R &rhs) {
            using bool_type = std::integral_constant<bool, N != 0>;
            static_if<bool_type{}>([](auto &l, auto &r) {
                tuple_helper::move_tuple_impl<N, 0>(l, r);
            })(lhs, rhs);
        }

        template<class L, class... Args>
        void iterate_tuple(const std::tuple<Args...> &t, const L &l) {
            using tuple_type = std::tuple<Args...>;
            tuple_helper::iterator<std::tuple_size<tuple_type>::value - 1, Args...>()(t, l, false);
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
