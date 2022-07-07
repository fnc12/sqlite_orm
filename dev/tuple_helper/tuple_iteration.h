#pragma once

#include <type_traits>  //  std::index_sequence, std::make_index_sequence
#include <utility>  //  std::forward, std::move

#include "../functional/cxx_universal.h"
#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"
#include "../functional/index_sequence_util.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            template<class... X>
            struct pack;

            template<class... X>
            struct uple;
        }

        //  got it form here https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
        template<class Function, class FunctionPointer, class Tuple, size_t... I>
        auto call_impl(Function& f, FunctionPointer functionPointer, Tuple t, std::index_sequence<I...>) {
            return (f.*functionPointer)(std::get<I>(std::move(t))...);
        }

        template<class Function, class FunctionPointer, class Tuple>
        auto call(Function& f, FunctionPointer functionPointer, Tuple t) {
            constexpr size_t size = std::tuple_size<Tuple>::value;
            return call_impl(f, functionPointer, std::move(t), std::make_index_sequence<size>{});
        }

        template<class Function, class Tuple>
        auto call(Function& f, Tuple t) {
            return call(f, &Function::operator(), std::move(t));
        }

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<bool reversed = false, class Tpl, size_t... Ix, class L>
        void iterate_tuple(const Tpl& tpl, std::index_sequence<Ix...>, L&& lambda) {
            if constexpr(reversed) {
                iterate_tuple(tpl, mpl::reverse_index_sequence(std::index_sequence<Ix...>{}), std::forward<L>(lambda));
            } else {
                (lambda(std::get<Ix>(tpl)), ...);
            }
        }
#else
        template<bool reversed = false, class Tpl, class L>
        void iterate_tuple(const Tpl& /*tpl*/, std::index_sequence<>, L&& /*lambda*/) {}

        template<bool reversed = false, class Tpl, size_t I, size_t... Ix, class L>
        void iterate_tuple(const Tpl& tpl, std::index_sequence<I, Ix...>, L&& lambda) {
#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
            if constexpr(reversed) {
#else
            if(reversed) {
#endif
                iterate_tuple<reversed>(tpl, std::index_sequence<Ix...>{}, std::forward<L>(lambda));
                lambda(std::get<I>(tpl));
            } else {
                int poormansfold[] = {(lambda(std::get<I>(tpl)), int{}), (lambda(std::get<Ix>(tpl)), int{})...};
                (void)poormansfold;
            }
        }
#endif
        template<bool reversed = false, class Tpl, class L>
        void iterate_tuple(const Tpl& tpl, L&& lambda) {
            iterate_tuple<reversed>(tpl,
                                    std::make_index_sequence<std::tuple_size<Tpl>::value>{},
                                    std::forward<L>(lambda));
        }

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        template<class... X, class L>
        void iterate_tuple(const mpl::uple<X...>& tpl, L&& lambda) {
            (lambda(std::get<X>(tpl)), ...);
        }
#else
        template<class... X, class L>
        void iterate_tuple(const mpl::uple<X...>& tpl, L&& lambda) {
            int poormansfold[] = {int{}, (lambda(std::get<X>(tpl)), int{})...};
            (void)poormansfold;
        }
#endif

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        template<class... X, class L>
        void iterate_pack(mpl::pack<X...>, L&& lambda) {
            (lambda((X*)nullptr), ...);
        }
#else
        template<class... X, class L>
        void iterate_pack(mpl::pack<X...>, L&& lambda) {
            int poormansfold[] = {int{}, (lambda((X*)nullptr), int{})...};
            (void)poormansfold;
        }
#endif

        template<class R, class Tpl, size_t... Ix, class Projection = polyfill::identity>
        R create_from_tuple(Tpl&& tpl, std::index_sequence<Ix...>, Projection project = {}) {
            return R{polyfill::invoke(project, std::get<Ix>(std::forward<Tpl>(tpl)))...};
        }

        template<class R, class Tpl, class Projection = polyfill::identity>
        R create_from_tuple(Tpl&& tpl, Projection project = {}) {
            return create_from_tuple<R>(
                std::forward<Tpl>(tpl),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tpl>>::value>{},
                std::forward<Projection>(project));
        }

        template<template<class...> class Base, class L>
        struct lambda_as_template_base : L {
#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            lambda_as_template_base(L&& lambda) : L{std::move(lambda)} {}
#endif
            template<class... T>
            decltype(auto) operator()(const Base<T...>& object) {
                return L::operator()(object);
            }
        };

        /*
         *  This method wraps the specified callable in another function object,
         *  which in turn implicitly casts its single argument to the specified template base class,
         *  then passes the converted argument to the lambda.
         *  
         *  Note: This method is useful for reducing combinatorial instantiation of template lambdas,
         *  as long as this library supports compilers that do not implement
         *  explicit template parameters in generic lambdas [SQLITE_ORM_EXPLICIT_GENERIC_LAMBDA_SUPPORTED].
         *  Unfortunately it doesn't work with user-defined conversion operators in order to extract
         *  parts of a class. In other words, the destination type must be a direct template base class.
         */
        template<template<class...> class Base, class L>
        lambda_as_template_base<Base, L> call_as_template_base(L lambda) {
            return {std::move(lambda)};
        }
    }
}
