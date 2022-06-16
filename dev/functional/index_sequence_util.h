#pragma once

#include <type_traits>  //  std::index_sequence, std::make_index_sequence

#include "cxx_universal.h"
#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
#include <array>
#endif
#include "pack.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            /**
             *  Get the first value of an index_sequence.
             */
            template<size_t I, size_t... Idx>
            SQLITE_ORM_CONSTEVAL size_t first_index_sequence_value(std::index_sequence<I, Idx...>) {
                return I;
            }

#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
            /**
             *  Reorder the values of an index_sequence according to the positions from a second sequence.
             */
            template<size_t... Value, size_t... IdxOfValue>
            SQLITE_ORM_CONSTEVAL auto reorder_index_sequence(std::index_sequence<Value...>,
                                                             std::index_sequence<IdxOfValue...>) {
                constexpr std::array<size_t, sizeof...(Value)> values{Value...};
                return std::index_sequence<values[sizeof...(Value) - 1u - IdxOfValue]...>{};
            }

            template<size_t Value, size_t IdxOfValue>
            SQLITE_ORM_CONSTEVAL std::index_sequence<Value> reorder_index_sequence(std::index_sequence<Value>,
                                                                                   std::index_sequence<IdxOfValue>) {
                return {};
            }

            inline SQLITE_ORM_CONSTEVAL std::index_sequence<> reorder_index_sequence(std::index_sequence<>,
                                                                                     std::index_sequence<>) {
                return {};
            }

            /**
             *  Reverse the values of an index_sequence.
             */
            template<size_t... Idx>
            SQLITE_ORM_CONSTEVAL auto reverse_index_sequence(std::index_sequence<Idx...>) {
                return reorder_index_sequence(std::index_sequence<Idx...>{},
                                              std::make_index_sequence<sizeof...(Idx)>{});
            }
#endif

#ifdef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
            template<size_t x, size_t c>
            using comma_expression_helper = std::integral_constant<size_t, c>;
#endif

            template<size_t n, size_t... Times>
            constexpr auto expand_n(std::index_sequence<Times...>) {
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
                using type = std::index_sequence<(Times, n)...>;
#else
                using type = std::index_sequence<comma_expression_helper<Times, n>::value...>;
#endif
                return type{};
            }

            template<size_t n, class Times>
            using expand_n_t = decltype(expand_n<n>(Times{}));

#ifdef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
            template<size_t x, size_t n>
            struct spread_idxseq_helper {
                using type = expand_n_t<x, std::make_index_sequence<n>>;
            };
#endif

            template<size_t... Ix, size_t... N>
            constexpr auto spread_idxseq(std::index_sequence<Ix...>, std::index_sequence<N...>) {
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
                using type = pack<expand_n_t<Ix, std::make_index_sequence<N>>...>;
#else
                using type = pack<typename spread_idxseq_helper<Ix, N>::type...>;
#endif
                return type{};
            }

            template<class Indices, class Sizes>
            using spread_idxseq_t = decltype(spread_idxseq(std::declval<Indices>(), std::declval<Sizes>()));

            template<class... Seq>
            struct flatten_idxseq {
                using type = std::index_sequence<>;
            };

            template<size_t... Idx>
            struct flatten_idxseq<std::index_sequence<Idx...>> {
                using type = std::index_sequence<Idx...>;
            };

            template<size_t... As, size_t... Bs, class... Seq>
            struct flatten_idxseq<std::index_sequence<As...>, std::index_sequence<Bs...>, Seq...>
                : flatten_idxseq<std::index_sequence<As..., Bs...>, Seq...> {};

            template<class... Seq>
            struct flatten_idxseq<pack<Seq...>> : flatten_idxseq<Seq...> {};

            template<class... Seq>
            using flatten_idxseq_t = typename flatten_idxseq<Seq...>::type;
        }
    }
}
