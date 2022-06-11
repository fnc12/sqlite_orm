#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::make_index_sequence
#include <tuple>

#include "cxx_universal.h"
#include "pack.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

#ifndef SQLITE_ORM_HAS_TYPE_PACK_ELEMENT_INTRINSIC
            template<typename Indices, typename... T>
            struct indexer;

            template<size_t... Idx, typename... T>
            struct indexer<std::index_sequence<Idx...>, T...> : indexed_type<Idx, T>... {};
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
            struct type_at<n, std::tuple<T...>> : type_at<n, T...> {};

            template<size_t n, typename... T>
            struct type_at<n, pack<T...>> : type_at<n, T...> {};

            template<size_t n, typename... T>
            using type_at_t = typename type_at<n, T...>::type;

            template<size_t n, typename Tpl>
            using element_at_t = typename type_at<n, Tpl>::type;

            template<typename... T>
            struct pack<std::tuple<T...>> : pack<T...> {};
        }
    }

    namespace mpl = internal::mpl;
}

// retain stl tuple interface for `tuple`
namespace std {
    template<class... X>
    struct tuple_size<sqlite_orm::mpl::pack<X...>> : integral_constant<size_t, sizeof...(X)> {};
}
