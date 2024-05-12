#pragma once

#include <utility>  //  std::index_sequence

#include "../functional/cxx_universal.h"  //  ::size_t

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
         *  Get the index value of an index_sequence at a specific position.
         */
        template<size_t... Idx>
        SQLITE_ORM_CONSTEVAL size_t index_sequence_value(size_t pos, std::index_sequence<Idx...>) {
            static_assert(sizeof...(Idx) > 0);
#ifdef SQLITE_ORM_PACK_INDEXING_SUPPORTED
            return Idx...[pos];
#else
#ifdef SQLITE_ORM_CONSTEVAL_SUPPORTED
            size_t result;
#else
            size_t result = 0;
#endif
            size_t i = 0;
            // note: `(void)` cast silences warning 'expression result unused'
            (void)((result = Idx, i++ == pos) || ...);
            return result;
#endif
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
