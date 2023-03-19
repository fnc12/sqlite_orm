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
