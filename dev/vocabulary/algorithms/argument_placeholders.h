#pragma once

/** @file Substitution of `argument<I>` placeholders in a built-in function's declared return type.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple_element
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../functional/mpl.h"
#include "../node_fwd.h"  // argument

namespace sqlite_orm::internal {
    /*
     *  `R` with every `argument<I>` replaced by the result of invoking the quoted metafunction `ResolveQ`
     *  on the I-th element of `ArgsTuple`. The substitution is structural: a placeholder is replaced
     *  wherever it appears inside a class template specialization (`std::unique_ptr<argument<0>>`),
     *  anything else is left as is.
     */
    template<class R, class ArgsTuple, class ResolveQ>
    struct substitute_arguments : polyfill::type_identity<R> {};

    template<size_t I, class ArgsTuple, class ResolveQ>
    struct substitute_arguments<argument<I>, ArgsTuple, ResolveQ>
        : mpl::defer<ResolveQ, std::tuple_element_t<I, ArgsTuple>> {};

    template<template<class...> class Tmpl, class... Ts, class ArgsTuple, class ResolveQ>
    struct substitute_arguments<Tmpl<Ts...>, ArgsTuple, ResolveQ>
        : polyfill::type_identity<Tmpl<typename substitute_arguments<Ts, ArgsTuple, ResolveQ>::type...>> {};

    template<class R, class ArgsTuple, class ResolveQ>
    using substitute_arguments_t = typename substitute_arguments<R, ArgsTuple, ResolveQ>::type;
}
