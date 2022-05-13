#pragma once

#include <type_traits>  //  std::integral_constant, std::index_sequence, std::conditional, std::declval
#include <tuple>  //  std::tuple

#include "../functional/mpl.h"

namespace sqlite_orm {
    namespace internal {

        template<typename... input_t>
        using tuple_cat_t = decltype(std::tuple_cat(std::declval<input_t>()...));

        template<class... Args>
        struct conc_tuple {
            using type = tuple_cat_t<Args...>;
        };

        template<class... Args>
        using conc_tuple_t = typename conc_tuple<Args...>::type;

        template<class T, template<class... C> class Fn>
        struct tuple_filter;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class... Types, template<class... C> class Fn>
        struct tuple_filter<std::tuple<Types...>, Fn>
            : conc_tuple<std::conditional_t<Fn<Types>::value, std::tuple<Types>, std::tuple<>>...> {};
#else
        template<class T, template<class... C> class Fn, class SFINAE = void>
        struct tuple_filter_single;

        template<class T, template<class... C> class Fn>
        struct tuple_filter_single<T, Fn, std::enable_if_t<!Fn<T>::value>> {
            using type = std::tuple<>;
        };

        template<class T, template<class... C> class Fn>
        struct tuple_filter_single<T, Fn, std::enable_if_t<Fn<T>::value>> {
            using type = std::tuple<T>;
        };

        template<class... Types, template<class... C> class Fn>
        struct tuple_filter<std::tuple<Types...>, Fn> : conc_tuple<typename tuple_filter_single<Types, Fn>::type...> {};
#endif
        template<class Tpl, template<class... C> class Fn>
        using filter_tuple_t = typename tuple_filter<Tpl, Fn>::type;

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
            : concat_idx_seq<std::conditional_t<Pred<Proj<std::tuple_element_t<Idx, Tpl>>>::value,
                                                std::index_sequence<Idx>,
                                                std::index_sequence<>>...> {};
#else
        template<size_t Idx, class T, template<class...> class Pred, template<class...> class Proj, class SFINAE = void>
        struct tuple_seq_single;

        template<size_t Idx, class T, template<class...> class Pred, template<class...> class Proj>
        struct tuple_seq_single<Idx, T, Pred, Proj, std::enable_if_t<!Pred<Proj<T>>::value>> {
            using type = std::index_sequence<>;
        };

        template<size_t Idx, class T, template<class...> class Pred, template<class...> class Proj>
        struct tuple_seq_single<Idx, T, Pred, Proj, std::enable_if_t<Pred<Proj<T>>::value>> {
            using type = std::index_sequence<Idx>;
        };

        template<class Tpl, template<class...> class Pred, template<class...> class Proj, size_t... Idx>
        struct filter_tuple_sequence<Tpl, Pred, Proj, std::index_sequence<Idx...>>
            : concat_idx_seq<typename tuple_seq_single<Idx, std::tuple_element_t<Idx, Tpl>, Pred, Proj>::type...> {};
#endif

        template<class Tpl,
                 template<class...>
                 class Pred,
                 template<class...> class Proj = polyfill::type_identity_t,
                 class Seq = std::make_index_sequence<std::tuple_size<Tpl>::value>>
        using filter_tuple_sequence_t = typename filter_tuple_sequence<Tpl, Pred, Proj, Seq>::type;

        template<class T, template<class...> class Fn>
        struct count_tuple : std::integral_constant<int, filter_tuple_sequence_t<T, Fn>::size()> {};

        /*
         *  Metafunction class that checks whether a tuple contains a type with given trait.
         */
        template<template<class...> class TraitFn>
        using mpl_tuple_has_trait = mpl::bind_front_higherorder_fn<tuple_helper::tuple_has, TraitFn>;

        /*
         *  Metafunction class that checks whether a tuple contains given type.
         */
        template<class T>
        using mpl_tuple_has_type =
            mpl::bind_front_higherorder_fn<tuple_helper::tuple_has, mpl::bind_front_fn<std::is_same, T>::template fn>;

        /*
         *  Metafunction class that checks whether a tuple contains a given template.
         *  
         *  Note: we are using 2 small tricks:
         *  1. A template template parameter can be treated like a metafunction, so we can just "quote" a 'primary'
         *     template into the MPL system (e.g. `std::vector`).
         *  2. This metafunction class does the opposite of the trait function `is_specialization`:
         *     `is_specialization` tries to instantiate the primary template template parameter using the
         *     template parameters of a template type, then compares both instantiated types.
         *     Here instead, `pass_extracted_fn` extracts the template template parameter from a template type,
         *     then compares the resulting template template parameters.
         */
        template<template<class...> class Primary>
        using mpl_tuple_has_template = mpl::bind_front_higherorder_fn<
            tuple_helper::tuple_has,
            mpl::pass_extracted_fn<mpl::bind_front_fn<std::is_same, mpl::quote_fn<Primary>>>::template fn>;
    }
}
