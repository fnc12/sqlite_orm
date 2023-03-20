#pragma once

#include <utility>  //  std::index_sequence, std::make_index_sequence

#include "../functional/cxx_universal.h"
#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
#include <array>
#endif

namespace sqlite_orm {
    namespace internal {
        /**
         *  Get the first value of an index_sequence.
         */
        template<size_t I, size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t first_index_sequence_value(std::index_sequence<I, Idx...>) {
            return I;
        }

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
        /**
         *  Get the value of an index_sequence at a specific position.
         */
        template<size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t index_sequence_value(size_t pos, std::index_sequence<Idx...>) {
            static_assert(sizeof...(Idx) > 0);
            size_t result;
            size_t i = 0;
            ((result = Idx, i++ == pos) || ...);
            return result;
        }
#endif

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
            return reorder_index_sequence(std::index_sequence<Idx...>{}, std::make_index_sequence<sizeof...(Idx)>{});
        }
#endif

        template<class... Seq>
        struct flatten_idxseq {
            using type = std::index_sequence<>;
        };

        template<size_t... Ix>
        struct flatten_idxseq<std::index_sequence<Ix...>> {
            using type = std::index_sequence<Ix...>;
        };

        template<size_t... As, size_t... Bs, class... Seq>
        struct flatten_idxseq<std::index_sequence<As...>, std::index_sequence<Bs...>, Seq...>
            : flatten_idxseq<std::index_sequence<As..., Bs...>, Seq...> {};

        template<class... Seq>
        using flatten_idxseq_t = typename flatten_idxseq<Seq...>::type;
    }
}
