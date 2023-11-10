#pragma once

#include <type_traits>  //  std::remove_reference, std::common_type, std::index_sequence, std::make_index_sequence, std::forward, std::move, std::integral_constant, std::declval
#include <tuple>  //  std::tuple_size, std::get

#include "../functional/cxx_universal.h"  //  ::size_t
#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cxx_functional_polyfill.h"
#include "../functional/mpl.h"

namespace sqlite_orm {
    namespace internal {

        template<class Pack, template<class...> class Op>
        struct tuple_transformer;

        template<template<class...> class Pack, class... Types, template<class...> class Op>
        struct tuple_transformer<Pack<Types...>, Op> {
            using type = Pack<mpl::invoke_fn_t<Op, Types>...>;
        };

        /*
         *  Transform specified tuple.
         *  
         *  `Op` is a metafunction.
         */
        template<class Pack, template<class...> class Op>
        using transform_tuple_t = typename tuple_transformer<Pack, Op>::type;

        //  note: applying a combiner like `plus_fold_integrals` needs fold expressions
#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
        /*
         *  Apply a projection to a tuple's elements filtered by the specified indexes, and combine the results.
         *  
         *  @note It's a glorified version of `std::apply()` and a variant of `std::accumulate()`.
         *  It combines filtering the tuple (indexes), transforming the elements (projection) and finally applying the callable (combine).
         *  
         *  @note `project` is called using `std::invoke`, which is `constexpr` since C++20.
         */
        template<class CombineOp, class Tpl, size_t... Idx, class Projector, class Init>
        SQLITE_ORM_CONSTEXPR_CPP20 auto recombine_tuple(CombineOp combine,
                                                        const Tpl& tpl,
                                                        std::index_sequence<Idx...>,
                                                        Projector project,
                                                        Init initial) {
            return combine(initial, polyfill::invoke(project, std::get<Idx>(tpl))...);
        }

        /*
         *  Apply a projection to a tuple's elements, and combine the results.
         *  
         *  @note It's a glorified version of `std::apply()` and a variant of `std::accumulate()`.
         *  It combines filtering the tuple (indexes), transforming the elements (projection) and finally applying the callable (combine).
         *  
         *  @note `project` is called using `std::invoke`, which is `constexpr` since C++20.
         */
        template<class CombineOp, class Tpl, class Projector, class Init>
        SQLITE_ORM_CONSTEXPR_CPP20 auto
        recombine_tuple(CombineOp combine, const Tpl& tpl, Projector project, Init initial) {
            return recombine_tuple(std::move(combine),
                                   std::forward<decltype(tpl)>(tpl),
                                   std::make_index_sequence<std::tuple_size<Tpl>::value>{},
                                   std::move(project),
                                   std::move(initial));
        }

        /*
         *  Function object that takes integral constants and returns the sum of their values as an integral constant.
         *  Because it's a "transparent" functor, it must be called with at least one argument, otherwise it cannot deduce the integral constant type.
         */
        struct plus_fold_integrals {
            template<class... Integrals>
            constexpr auto operator()(const Integrals&...) const {
                using integral_type = std::common_type_t<typename Integrals::value_type...>;
                return std::integral_constant<integral_type, (Integrals::value + ...)>{};
            }
        };

        /*
         *  Function object that takes a type, applies a projection on it, and returns the tuple size of the projected type (as an integral constant).
         *  The projection is applied on the argument type, not the argument value/object.
         */
        template<template<class...> class NestedProject>
        struct project_nested_tuple_size {
            template<class T>
            constexpr auto operator()(const T&) const {
                return typename std::tuple_size<NestedProject<T>>::type{};
            }
        };

        template<template<class...> class NestedProject, class Tpl, class IdxSeq>
        using nested_tuple_size_for_t = decltype(recombine_tuple(plus_fold_integrals{},
                                                                 std::declval<Tpl>(),
                                                                 IdxSeq{},
                                                                 project_nested_tuple_size<NestedProject>{},
                                                                 std::integral_constant<size_t, 0u>{}));
#endif

        template<class R, class Tpl, size_t... Idx, class Projection = polyfill::identity>
        constexpr R create_from_tuple(Tpl&& tpl, std::index_sequence<Idx...>, Projection project = {}) {
            return R{polyfill::invoke(project, std::get<Idx>(std::forward<Tpl>(tpl)))...};
        }

        template<class R, class Tpl, class Projection = polyfill::identity>
        constexpr R create_from_tuple(Tpl&& tpl, Projection project = {}) {
            return create_from_tuple<R>(
                std::forward<Tpl>(tpl),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tpl>>::value>{},
                std::forward<Projection>(project));
        }
    }
}
