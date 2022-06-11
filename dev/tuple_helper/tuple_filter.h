#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::make_index_sequence, std::conditional, std::declval

#include "../functional/cxx_universal.h"
#include "../functional/type_at.h"
#include "../functional/pack.h"
#include "../functional/unique_tuple.h"
#include "../functional/tuple.h"

namespace sqlite_orm {
    namespace internal {

        template<typename... input_t>
        using tuple_cat_t = decltype(std::tuple_cat(std::declval<input_t>()...));

        template<class... Tpl>
        struct conc_tuple {
            using type = tuple_cat_t<Tpl...>;
        };

        template<class... Tpl>
        using conc_tuple_t = typename conc_tuple<Tpl...>::type;

        template<class Tpl, class Seq>
        struct tuple_from_index_sequence;

        template<template<class...> class Tuple, class... T, size_t... Idx>
        struct tuple_from_index_sequence<Tuple<T...>, std::index_sequence<Idx...>> {
            using type = Tuple<mpl::element_at_t<Idx, Tuple<T...>>...>;
        };

        template<class Tpl, class Seq>
        using tuple_from_index_sequence_t = typename tuple_from_index_sequence<Tpl, Seq>::type;

        template<class... Seq>
        struct concat_idx_seq {
            using type = std::index_sequence<>;
        };

        template<size_t... Idx>
        struct concat_idx_seq<std::index_sequence<Idx...>> {
            using type = std::index_sequence<Idx...>;
        };

        template<size_t... As, size_t... Bs, class... Seq>
        struct concat_idx_seq<std::index_sequence<As...>, std::index_sequence<Bs...>, Seq...>
            : concat_idx_seq<std::index_sequence<As..., Bs...>, Seq...> {};

        template<class Tpl, template<class...> class Pred, template<class...> class Proj, class Seq>
        struct filter_tuple_sequence;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class Tpl, template<class...> class Pred, template<class...> class Proj, size_t... Idx>
        struct filter_tuple_sequence<Tpl, Pred, Proj, std::index_sequence<Idx...>>
            : concat_idx_seq<std::conditional_t<Pred<Proj<mpl::element_at_t<Idx, Tpl>>>::value,
                                                std::index_sequence<Idx>,
                                                std::index_sequence<>>...> {};
#else
        template<size_t Idx, class T, template<class...> class Pred, class SFINAE = void>
        struct tuple_seq_single {
            using type = std::index_sequence<>;
        };

        template<size_t Idx, class T, template<class...> class Pred>
        struct tuple_seq_single<Idx, T, Pred, std::enable_if_t<Pred<T>::value>> {
            using type = std::index_sequence<Idx>;
        };

        template<class Tpl, template<class...> class Pred, template<class...> class Proj, size_t... Idx>
        struct filter_tuple_sequence<Tpl, Pred, Proj, std::index_sequence<Idx...>>
            : concat_idx_seq<typename tuple_seq_single<Idx, Proj<mpl::element_at_t<Idx, Tpl>>, Pred>::type...> {};
#endif

        template<class Tpl,
                 template<class...>
                 class Pred,
                 template<class...> class Proj = polyfill::type_identity_t,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using filter_tuple_sequence_t = typename filter_tuple_sequence<Tpl, Pred, Proj, Seq>::type;

        template<class Tpl,
                 template<class...>
                 class Pred,
                 template<class...> class FilterProj = polyfill::type_identity_t,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using filter_tuple_t = tuple_from_index_sequence_t<Tpl, filter_tuple_sequence_t<Tpl, Pred, FilterProj, Seq>>;

        template<class Tpl,
                 template<class...>
                 class Pred,
                 template<class...> class FilterProj = polyfill::type_identity_t>
        struct count_tuple : std::integral_constant<int, filter_tuple_sequence_t<Tpl, Pred, FilterProj>::size()> {};

        /*
         *  Count a tuple, picking only those elements specified in the index sequence.
         *  
         *  Implementation note: must be distinct from `count_tuple` because legacy compilers have problems
         *  with a default Sequence in function template parameters [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
         */
        template<class Tpl,
                 template<class...>
                 class Pred,
                 class Seq,
                 template<class...> class FilterProj = polyfill::type_identity_t>
        struct count_filtered_tuple
            : std::integral_constant<size_t, filter_tuple_sequence_t<Tpl, Pred, FilterProj, Seq>::size()> {};
    }
}
