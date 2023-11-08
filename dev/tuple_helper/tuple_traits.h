#pragma once

#include <type_traits>  //  std::is_same
#include <tuple>

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"

namespace sqlite_orm {
    namespace internal {
        /*
         *  Higher-order trait metafunction that checks whether a tuple contains a type with given trait.
         */
        template<template<class...> class TraitFn, class Tuple>
        struct tuple_has {};
        template<template<class...> class TraitFn, class... Types>
        struct tuple_has<TraitFn, std::tuple<Types...>> : polyfill::disjunction<TraitFn<Types>...> {};

        /*
         *  Trait metafunction class that checks whether a tuple contains a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_tuple_has = mpl::bind_front_higherorder_fn<tuple_has, TraitFn>;

        /*
         *  Trait metafunction class that checks whether a tuple doesn't contain a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_tuple_has_not = mpl::not_<check_if_tuple_has<TraitFn>>;

        /*
         *  Metafunction class that checks whether a tuple contains given type.
         */
        template<class T, template<class...> class Proj = polyfill::type_identity_t>
        using check_if_tuple_has_type =
            mpl::bind_front_higherorder_fn<tuple_has, check_if_is_type<T, Proj>::template fn>;

        /*
         *  Metafunction class that checks whether a tuple contains a given template.
         *
         *  Note: we are using 2 small tricks:
         *  1. A template template parameter can be treated like a metafunction, so we can just "quote" a 'primary'
         *     template into the MPL system (e.g. `std::vector`).
         *  2. This metafunction class does the opposite of the trait function `is_specialization`:
         *     `is_specialization` tries to instantiate the primary template template parameter using the
         *     template parameters of a template type, then compares both instantiated types.
         *     Here instead, `pass_extracted_fn_to` extracts the template template parameter from a template type,
         *     then compares the resulting template template parameters.
         */
        template<template<class...> class Primary>
        using check_if_tuple_has_template =
            mpl::bind_front_higherorder_fn<tuple_has, check_if_is_template<Primary>::template fn>;
    }
}