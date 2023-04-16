#pragma once

#include <tuple>  //  std::tuple, std::get, std::tuple_element, std::tuple_size
#include <type_traits>  //  std::remove_reference, std::index_sequence, std::make_index_sequence
#include <utility>  //  std::forward, std::move

#include "../functional/cxx_universal.h"  //  ::size_t
#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"

namespace sqlite_orm {
    namespace internal {

        //  got it form here https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
        template<class Function, class FunctionPointer, class Tpl, size_t... Idx>
        auto call(Function& f, FunctionPointer functionPointer, Tpl&& tpl, std::index_sequence<Idx...>) {
            return (f.*functionPointer)(std::get<Idx>(std::forward<Tpl>(tpl))...);
        }

        template<class Function, class Tpl, size_t... Idx>
        auto call(Function& f, Tpl&& tpl, std::index_sequence<Idx...>) {
            return f(std::get<Idx>(std::forward<Tpl>(tpl))...);
        }

        template<class Function, class FunctionPointer, class Tpl>
        auto call(Function& f, FunctionPointer functionPointer, Tpl&& tpl) {
            constexpr size_t size = std::tuple_size<std::remove_reference_t<Tpl>>::value;
            return call(f, functionPointer, std::forward<Tpl>(tpl), std::make_index_sequence<size>{});
        }

        // custom std::apply
        template<class Function, class Tpl>
        auto call(Function& f, Tpl&& tpl) {
            constexpr size_t size = std::tuple_size<std::remove_reference_t<Tpl>>::value;
            return call(f, std::forward<Tpl>(tpl), std::make_index_sequence<size>{});
        }

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<bool reversed = false, class Tpl, size_t... Idx, class L>
        void iterate_tuple(const Tpl& tpl, std::index_sequence<Idx...>, L&& lambda) {
            if constexpr(reversed) {
                // nifty fold expression trick: make use of guaranteed right-to-left evaluation order when folding over operator=
                int sink;
                ((lambda(std::get<Idx>(tpl)), sink) = ... = 0);
            } else {
                (lambda(std::get<Idx>(tpl)), ...);
            }
        }
#else
        template<bool reversed = false, class Tpl, class L>
        void iterate_tuple(const Tpl& /*tpl*/, std::index_sequence<>, L&& /*lambda*/) {}

        template<bool reversed = false, class Tpl, size_t I, size_t... Idx, class L>
        void iterate_tuple(const Tpl& tpl, std::index_sequence<I, Idx...>, L&& lambda) {
#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
            if constexpr(reversed) {
#else
            if(reversed) {
#endif
                iterate_tuple<reversed>(tpl, std::index_sequence<Idx...>{}, std::forward<L>(lambda));
                lambda(std::get<I>(tpl));
            } else {
                lambda(std::get<I>(tpl));
                iterate_tuple<reversed>(tpl, std::index_sequence<Idx...>{}, std::forward<L>(lambda));
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
        template<class Tpl, size_t... Idx, class L>
        void iterate_tuple(std::index_sequence<Idx...>, L&& lambda) {
            (lambda((std::tuple_element_t<Idx, Tpl>*)nullptr), ...);
        }
#else
        template<class Tpl, class L>
        void iterate_tuple(std::index_sequence<>, L&& /*lambda*/) {}

        template<class Tpl, size_t I, size_t... Idx, class L>
        void iterate_tuple(std::index_sequence<I, Idx...>, L&& lambda) {
            lambda((std::tuple_element_t<I, Tpl>*)nullptr);
            iterate_tuple<Tpl>(std::index_sequence<Idx...>{}, std::forward<L>(lambda));
        }
#endif
        template<class Tpl, class L>
        void iterate_tuple(L&& lambda) {
            iterate_tuple<Tpl>(std::make_index_sequence<std::tuple_size<Tpl>::value>{}, std::forward<L>(lambda));
        }

        template<class R, class Tpl, size_t... Idx, class Projection = polyfill::identity>
        R create_from_tuple(Tpl&& tpl, std::index_sequence<Idx...>, Projection project = {}) {
            return R{polyfill::invoke(project, std::get<Idx>(std::forward<Tpl>(tpl)))...};
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
