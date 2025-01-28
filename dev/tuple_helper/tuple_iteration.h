#pragma once

#ifndef _IMPORT_STD_MODULE
#include <tuple>  //  std::get, std::tuple_element, std::tuple_size
#include <type_traits>  //  std::index_sequence, std::make_index_sequence
#include <utility>  //  std::forward, std::move
#endif

namespace sqlite_orm {
    namespace internal {
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
        template<bool reversed = false, class Tpl, size_t... Idx, class L>
        constexpr void iterate_tuple(Tpl& tpl, std::index_sequence<Idx...>, L&& lambda) {
            if constexpr (reversed) {
                // nifty fold expression trick: make use of guaranteed right-to-left evaluation order when folding over operator=
                int sink;
                // note: `(void)` cast silences warning 'expression result unused'
                (void)((lambda(std::get<Idx>(tpl)), sink) = ... = 0);
            } else {
                (lambda(std::get<Idx>(tpl)), ...);
            }
        }
#else
        template<bool reversed = false, class Tpl, class L>
        void iterate_tuple(Tpl& /*tpl*/, std::index_sequence<>, L&& /*lambda*/) {}

        template<bool reversed = false, class Tpl, size_t I, size_t... Idx, class L>
        void iterate_tuple(Tpl& tpl, std::index_sequence<I, Idx...>, L&& lambda) {
            if SQLITE_ORM_CONSTEXPR_IF (reversed) {
                iterate_tuple<reversed>(tpl, std::index_sequence<Idx...>{}, std::forward<L>(lambda));
                lambda(std::get<I>(tpl));
            } else {
                lambda(std::get<I>(tpl));
                iterate_tuple<reversed>(tpl, std::index_sequence<Idx...>{}, std::forward<L>(lambda));
            }
        }
#endif
        template<bool reversed = false, class Tpl, class L>
        constexpr void iterate_tuple(Tpl&& tpl, L&& lambda) {
            iterate_tuple<reversed>(tpl,
                                    std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tpl>>::value>{},
                                    std::forward<L>(lambda));
        }

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        template<class Tpl, size_t... Idx, class L>
        void iterate_tuple(std::index_sequence<Idx...>, L&& lambda) {
            (lambda((std::tuple_element_t<Idx, Tpl>*)nullptr), ...);
        }
#else
        template<class Tpl, size_t... Idx, class L>
        void iterate_tuple(std::index_sequence<Idx...>, L&& lambda) {
            using Sink = int[sizeof...(Idx)];
            (void)Sink{(lambda((std::tuple_element_t<Idx, Tpl>*)nullptr), 0)...};
        }
#endif
        template<class Tpl, class L>
        void iterate_tuple(L&& lambda) {
            iterate_tuple<Tpl>(std::make_index_sequence<std::tuple_size<Tpl>::value>{}, std::forward<L>(lambda));
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
