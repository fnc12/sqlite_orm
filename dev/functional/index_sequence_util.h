#pragma once

#include <utility>  //  std::index_sequence

#include "../functional/cxx_universal.h"  //  ::size_t

namespace sqlite_orm {
    namespace internal {
#if defined(SQLITE_ORM_PACK_INDEXING_SUPPORTED)
        /**
         *  Get the index value of an `index_sequence` at a specific position.
         */
        template<size_t Pos, size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t index_sequence_value_at(std::index_sequence<Idx...>) {
            return Idx...[Pos];
        }
#elif defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED)
        /**
         *  Get the index value of an `index_sequence` at a specific position.
         */
        template<size_t Pos, size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t index_sequence_value_at(std::index_sequence<Idx...>) {
            SQLITE_ORM_STASSERT(Pos < sizeof...(Idx));
#ifdef SQLITE_ORM_CONSTEVAL_SUPPORTED
            size_t result;
#else
            size_t result = 0;
#endif
            size_t i = 0;
            // note: `(void)` cast silences warning 'expression result unused'
            (void)((result = Idx, i++ == Pos) || ...);
            return result;
        }
#else
        /**
         *  Get the index value of an `index_sequence` at a specific position.
         *  `Pos` must always be `0`.
         */
        template<size_t Pos, size_t I, size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t index_sequence_value_at(std::index_sequence<I, Idx...>) {
            SQLITE_ORM_STASSERT(Pos == 0, "");
            return I;
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
