#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::make_index_sequence, std::conditional, std::declval

#include "../functional/cxx_universal.h"
#include "../functional/index_sequence_util.h"
#include "../functional/pack_util.h"
#include "../functional/tuple.h"

namespace sqlite_orm {
    namespace internal {

        template<typename... Tpl>
        using tuple_cat_t = mpl::flatten_types_t<mpl::tuple, Tpl...>;

        template<class... Tpl>
        struct conc_tuple : mpl::flatten_types<mpl::tuple, Tpl...> {};

        template<class Tpl, class Seq>
        struct tuple_from_index_sequence;

        template<template<class...> class Tuple, class... T, size_t... Idx>
        struct tuple_from_index_sequence<Tuple<T...>, std::index_sequence<Idx...>> {
            using type = Tuple<mpl::element_at_t<Idx, Tuple<T...>>...>;
        };

        template<class Tpl, class Seq>
        using tuple_from_index_sequence_t = typename tuple_from_index_sequence<Tpl, Seq>::type;

        template<class Tpl, template<class...> class Pred, template<class...> class Proj, class Seq>
        struct filter_tuple_sequence;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class Tpl, template<class...> class Pred, template<class...> class Proj, size_t... Idx>
        struct filter_tuple_sequence<Tpl, Pred, Proj, std::index_sequence<Idx...>>
            : mpl::flatten_idxseq<std::conditional_t<Pred<Proj<mpl::element_at_t<Idx, Tpl>>>::value,
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
            : mpl::flatten_idxseq<typename tuple_seq_single<Idx, Proj<mpl::element_at_t<Idx, Tpl>>, Pred>::type...> {};
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
