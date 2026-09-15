#pragma once

/** @file Substitution of the `argument<I>` and `common_argument_type<I...>` placeholders
 *        in a built-in function's declared return type.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::tuple_element
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/mpl.h"
#include "../../functional/type_traits.h"  //  common_type_of

namespace sqlite_orm::internal {
    /*
     *  Placeholder in the return type of a built-in function's signature,
     *  standing for the result type of the I-th call argument.
     *
     *  It may appear anywhere inside the return type, e.g. `std::unique_ptr<argument<0>>`.
     */
    template<size_t I>
    struct argument {};

    /*
     *  Placeholder in the return type of a built-in function's signature,
     *  standing for the common type of the result types of the call arguments with the given indexes,
     *  e.g. `common_argument_type<1, 2>`, or of all call arguments if no index is given: `common_argument_type<>`.
     */
    template<size_t... I>
    struct common_argument_type {};

    /*
     *  `R` with every `argument<I>` replaced by the result of invoking the quoted metafunction `ResolveQ`
     *  on the I-th element of `ArgsTuple`, and every `common_argument_type<I...>` by the common type of those.
     *  The substitution is structural: a placeholder is replaced wherever it appears inside a class template
     *  specialization (`std::unique_ptr<argument<0>>`), anything else is left as is.
     */
    template<class R, class ArgsTuple, class ResolveQ>
    struct substitute_arguments : polyfill::type_identity<R> {};

    template<size_t I, class ArgsTuple, class ResolveQ>
    struct substitute_arguments<argument<I>, ArgsTuple, ResolveQ>
        : mpl::defer<ResolveQ, std::tuple_element_t<I, ArgsTuple>> {};

    template<template<class...> class Tmpl, class... Ts, class ArgsTuple, class ResolveQ>
    struct substitute_arguments<Tmpl<Ts...>, ArgsTuple, ResolveQ>
        : polyfill::type_identity<Tmpl<typename substitute_arguments<Ts, ArgsTuple, ResolveQ>::type...>> {};

    template<size_t... I, class ArgsTuple, class ResolveQ>
    struct substitute_arguments<common_argument_type<I...>, ArgsTuple, ResolveQ>
        : common_type_of<std::tuple<mpl::invoke_t<ResolveQ, std::tuple_element_t<I, ArgsTuple>>...>> {};

    template<class... Args, class ResolveQ>
    struct substitute_arguments<common_argument_type<>, std::tuple<Args...>, ResolveQ>
        : common_type_of<std::tuple<mpl::invoke_t<ResolveQ, Args>...>> {};

    template<class R, class ArgsTuple, class ResolveQ>
    using substitute_arguments_t = typename substitute_arguments<R, ArgsTuple, ResolveQ>::type;
}
