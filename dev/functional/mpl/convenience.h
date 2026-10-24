#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../cxx_polyfill/type_traits.h"  //  polyfill::is_detected
#include "quoted_metafunctions.h"

namespace sqlite_orm::internal::mpl {
    /*
     *  Convenience quoted metafunctions composed from the mpl facilities.
     */
    inline namespace convenience {
        /*
         *  Quoted trait metafunction that checks if a type has the specified trait.
         */
        template<template<class...> class TraitFn, class... Bound>
        using check_if = conditional_t<sizeof...(Bound) == 0, quote_fn<TraitFn>, bind_front_fn<TraitFn, Bound...>>;

        /*
         *  Quoted trait metafunction that checks if a type doesn't have the specified trait.
         */
        template<template<class...> class TraitFn>
        using check_if_not = not_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks if a type is the same as the specified type.
         *  Commonly used named abbreviation for `check_if<std::is_same, Type>`.
         */
        template<class Type>
        using check_if_is_type = check_if<std::is_same, Type>;

        /*
         *  Quoted trait metafunction that checks if a type's template matches the specified template
         *  (similar to `is_specialization_of`).
         */
        template<template<class...> class Template>
        using check_if_is_template = pass_extracted_fn_to<bind_front_fn<std::is_same, quote_fn<Template>>>;

        /*
         *  Quoted trait metafunction that checks if a type names a nested type determined by `Op`.
         */
        template<template<typename...> class Op>
        using check_if_names = bind_front_higherorder_fn<polyfill::is_detected, Op>;

        /*
         *  Quoted trait metafunction that checks if a type does not name a nested type determined by `Op`.
         */
        template<template<typename...> class Op>
        using check_if_lacks = not_<check_if_names<Op>>;

        /*
         *  Quoted metafunction that finds the index of the element having the specified trait in a tuple.
         */
        template<template<class...> class TraitFn>
        using finds_if_has = finds<check_if<TraitFn>>;

        /*
         *  Quoted metafunction that finds the index of the trait metafunction in a tuple that is satisfied by the given type.
         */
        template<class T>
        using finds_satisfied_by = finds<satisfied_by<T>>;

        /*
         *  Quoted metafunction that finds the index of the given type in a tuple.
         */
        template<class Type>
        using finds_if_has_type = finds<check_if_is_type<Type>>;

        /*
         *  Quoted metafunction that finds the index of the given class template in a tuple.
         */
        template<template<class...> class Template>
        using finds_if_has_template = finds<check_if_is_template<Template>>;

        /*
         *  Quoted trait metafunction that counts tuple elements having a given trait.
         */
        template<template<class...> class TraitFn>
        using counts_if_has = counts_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks whether a tuple contains a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_has = contains_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks whether a tuple doesn't contain a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_has_not = not_<contains_fn<TraitFn>>;

        /*
         *  Quoted trait metafunction that checks whether a tuple contains only types with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_has_only = not_<contains<check_if_not<TraitFn>>>;

        /*
         *  Quoted metafunction that checks whether a tuple contains given type.
         */
        template<class T>
        using check_if_has_type = contains<check_if_is_type<T>>;

        /*
         *  Quoted metafunction that checks whether a tuple contains a given template.
         *
         *  Note: we are using 2 small tricks:
         *  1. A template template parameter can be treated like a metafunction, so we can just "quote" a 'primary'
         *     template into the MPL system (e.g. `std::vector`).
         *  2. This quoted metafunction does the opposite of the trait metafunction `is_specialization`:
         *     `is_specialization` tries to instantiate the primary template template parameter using the
         *     template parameters of a template type, then compares both instantiated types.
         *     Here instead, `pass_extracted_fn_to` extracts the template template parameter from a template type,
         *     then compares the resulting template template parameters.
         */
        template<template<class...> class Template>
        using check_if_has_template = contains<check_if_is_template<Template>>;

        template<template<class...> class ProjOp, class T>
        using check_if_projected_is_type = pass_result_of_fn<check_if_is_type<T>, ProjOp>;

        /*
         *  Higher-order trait metafunction that checks whether a tuple contains a type with given trait (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...> class TraitFn,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has = invoke_t<check_if_has<TraitFn>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that checks whether a tuple contains the specified type (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack, class Type, template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has_type = invoke_t<check_if_has_type<Type>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that checks whether a tuple contains the specified class template (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...> class Template,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using tuple_has_template = invoke_t<check_if_has_template<Template>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the desired element having the specified trait in a tuple (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...> class TraitFn,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_element = invoke_t<finds_if_has<TraitFn>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the trait metafunction in a tuple
         *  that is satisfied by the specified type (possibly projected).
         *  
         *  In contrast to `find_tuple_element`, the tuple holds the trait metafunctions and the type to test is fixed,
         *  which is what allows a single type to be located within an ordered list of traits.
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack, class T, template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_satisfied_by = invoke_t<finds_satisfied_by<T>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the desired type in a tuple (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack, class Type, template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_type = invoke_t<finds_if_has_type<Type>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order metafunction returning the first index constant of the desired class template in a tuple (possibly projected).
         *  
         *  `ProjOp` is a metafunction
         */
        template<class Pack,
                 template<class...> class Template,
                 template<class...> class ProjOp = polyfill::type_identity_t>
        using find_tuple_template = invoke_t<finds_if_has_template<Template>, Pack, quote_fn<ProjOp>>;

        /*
         *  Higher-order trait metafunction that counts the types having the specified trait in a tuple (possibly projected).
         *  
         *  `Pred` is a predicate metafunction with a nested bool member named `value`
         *  `ProjOp` is a metafunction
         */
        template<class Pack, template<class...> class Pred, template<class...> class ProjOp = polyfill::type_identity_t>
        using count_tuple = invoke_t<counts_if_has<Pred>, Pack, quote_fn<ProjOp>>;
    }
}

namespace sqlite_orm::internal {
    using namespace mpl::convenience;
}
