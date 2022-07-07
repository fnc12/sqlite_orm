#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::make_index_sequence

#include "cxx_universal.h"
#include "indexed_type.h"
#include "pack.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

#ifndef SQLITE_ORM_HAS_TYPE_PACK_ELEMENT_INTRINSIC
            template<typename Indices, typename... T>
            struct indexer;

            template<size_t... Ix, typename... T>
            struct indexer<std::index_sequence<Ix...>, T...> : indexed_type<Ix, T>... {};

            template<size_t I, typename T>
            indexed_type<I, T> get_indexed_type(const indexed_type<I, T>&);
#endif

            template<size_t n, typename... T>
            struct type_at {
#ifdef SQLITE_ORM_HAS_TYPE_PACK_ELEMENT_INTRINSIC
                using type = __type_pack_element<n, T...>;
#else
                using Indexer = indexer<std::make_index_sequence<sizeof...(T)>, T...>;
                // implementation note: needs to be aliased on its own [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION]
                using indexed_t = decltype(get_indexed_type<n>(Indexer{}));
                using type = typename indexed_t::type;
#endif
            };

            template<size_t n, typename... T>
            struct type_at<n, pack<T...>> : type_at<n, T...> {};

            template<size_t n, typename... T>
            using type_at_t = typename type_at<n, T...>::type;

            template<size_t n, typename Tpl>
            using element_at_t = typename type_at<n, Tpl>::type;
        }
    }

    namespace mpl = internal::mpl;
}
