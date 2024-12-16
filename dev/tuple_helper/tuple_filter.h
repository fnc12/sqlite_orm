#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::conditional, std::declval
#include <tuple>  //  std::tuple, std::tuple_cat, std::tuple_element

#include "../functional/mpl/conditional.h"
#include "../functional/index_sequence_util.h"

namespace sqlite_orm {
    namespace internal {

        template<typename... input_t>
        using tuple_cat_t = decltype(std::tuple_cat(std::declval<input_t>()...));

        template<class... Tpl>
        struct conc_tuple {
            using type = tuple_cat_t<Tpl...>;
        };

        template<class Tpl, class Seq>
        struct tuple_from_index_sequence;

        template<class Tpl, size_t... Idx>
        struct tuple_from_index_sequence<Tpl, std::index_sequence<Idx...>> {
            using type = std::tuple<std::tuple_element_t<Idx, Tpl>...>;
        };

        template<class Tpl, class Seq>
        using tuple_from_index_sequence_t = typename tuple_from_index_sequence<Tpl, Seq>::type;

        template<class Tpl, template<class...> class Pred, template<class...> class Proj, class Seq>
        struct filter_tuple_sequence;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class Tpl, template<class...> class Pred, template<class...> class Proj, size_t... Idx>
        struct filter_tuple_sequence<Tpl, Pred, Proj, std::index_sequence<Idx...>>
            : flatten_idxseq<mpl::conditional_t<Pred<mpl::invoke_fn_t<Proj, std::tuple_element_t<Idx, Tpl>>>::value,
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
            : flatten_idxseq<typename tuple_seq_single<Idx,
                                                       mpl::invoke_fn_t<Proj, std::tuple_element_t<Idx, Tpl>>,
                                                       Pred>::type...> {};
#endif

        /*
         *  `Pred` is a metafunction that defines a bool member named `value`
         *  `FilterProj` is a metafunction
         */
        template<class Tpl,
                 template<class...> class Pred,
                 template<class...> class FilterProj = polyfill::type_identity_t,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using filter_tuple_sequence_t = typename filter_tuple_sequence<Tpl, Pred, FilterProj, Seq>::type;

        /*
         *  `Pred` is a metafunction that defines a bool member named `value`
         *  `FilterProj` is a metafunction
         */
        template<class Tpl,
                 template<class...> class Pred,
                 template<class...> class FilterProj = polyfill::type_identity_t,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using filter_tuple_t = tuple_from_index_sequence_t<Tpl, filter_tuple_sequence_t<Tpl, Pred, FilterProj, Seq>>;

        /*
         *  Count a tuple, picking only those elements specified in the index sequence.
         *  
         *  `Pred` is a metafunction that defines a bool member named `value`
         *  `FilterProj` is a metafunction
         *  
         *  Implementation note: must be distinct from a `count_tuple` w/o index sequence parameter because legacy compilers have problems
         *  with a default Sequence in function template parameters [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
         */
        template<class Tpl,
                 template<class...> class Pred,
                 class Seq,
                 template<class...> class FilterProj = polyfill::type_identity_t>
        struct count_filtered_tuple
            : std::integral_constant<size_t, filter_tuple_sequence_t<Tpl, Pred, FilterProj, Seq>::size()> {};
    }
}
