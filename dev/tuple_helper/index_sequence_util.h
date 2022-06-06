#pragma once

#include <type_traits>  //  std::index_sequence, std::make_index_sequence

#include "../functional/cxx_universal.h"
#ifdef SQLITE_ORM_RELAXED_CONSTEXPR
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

#ifdef SQLITE_ORM_RELAXED_CONSTEXPR
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
    }
}
