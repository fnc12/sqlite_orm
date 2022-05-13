#pragma once

/*
 *  Symbols for 'template metaprogramming' (compile-time template programming),
 *  inspired by the MPL of Aleksey Gurtovoy and David Abrahams'.
 *  
 *  Currently, the focus is on facilitating advanced type filtering, such as filtering columns by constraints.
 *  
 *  Two key concepts are critical to understanding:
 *  1. A 'metafunction' is a class template that represents a function invocable at compile-time.
 *  2. A 'metafunction class' is a certain form of metafunction representation that enables higher-order metaprogramming.
 *     More precisely, it's a class with a nested metafunction called "fn"
 *     Correspondingly, a metafunction class invocation is defined as invocation of its nested apply metafunction.
 *  
 *  Conventions:
 *  - "Fn" is the name for a metafunction template template parameter.
 *  - "FnCls" is the name for a metafunction class template parameter.
 *  - "_fn" is a suffix for a type that accepts metafunctions and turns them into metafunction classes.
 *  - "higher order" denotes a metafunction that operates on another metafunction (i.e. takes it as an argument).
 */

#include <type_traits>

#include "cxx_universal.h"
#include "cxx_polyfill.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            /*
             *  Wrap given type such that `typename T::type` is valid.
             */
            template<class T, class SFINAE = void>
            struct type_wrap : polyfill::type_identity<T> {};
            template<class T>
            struct type_wrap<T, polyfill::void_t<typename T::type>> : T {};

            /*
             *  Invoke metafunction.
             */
            template<template<class...> class Fn, class... Args>
            using invoke_fn_t = typename Fn<Args...>::type;

            /*
             *  Invoke metafunction class.
             */
            template<class FnCls, class... Args>
            using invoke_t = typename FnCls::template fn<Args...>::type;

            /*
             *  Make metafunction class out of a metafunction.
             */
            template<template<class...> class Fn>
            struct quote_fn {
                template<class... Args>
                struct fn : type_wrap<Fn<Args...>> {};
            };

            /*
             *  Make metafunction class out of a higher-order metafunction.
             */
            template<template<template<class...> class Fn, class... Args2> class HigherFn>
            struct quote_higherorder_front_fn {
                template<class QuotedFn, class... Args2>
                struct fn : type_wrap<HigherFn<typename QuotedFn::template fn, Args2...>> {};
            };

            /*
             *  Make metafunction class out of a higher-order metafunction having 2 arguments.
             */
            template<template<class, template<class...> class> class HigherFn>
            struct quote_2_higherorder_back_fn {
                template<class Arg1, class QuotedFn>
                struct fn : type_wrap<HigherFn<Arg1, typename QuotedFn::template fn>> {};
            };

            /*
             *  Bind arguments at the front of a metafunction class.
             *  Metafunction class equivalent to std::bind_front()
             */
            template<class FnCls, class... Bound>
            struct bind_front {
                template<class... Args>
                struct fn : FnCls::template fn<Bound..., Args...> {};
            };

            /*
             *  Bind arguments at the back of a metafunction class.
             *  Metafunction class equivalent to std::bind_back()
             */
            template<class FnCls, class... Bound>
            struct bind_back {
                template<class... Args>
                struct fn : FnCls::template fn<Args..., Bound...> {};
            };

            /*
             *  Metafunction class equivalent to std::negation
             */
            template<class FnCls>
            struct not_ {
                template<class... Args>
                struct fn : polyfill::negation<invoke_t<FnCls, Args...>> {};
            };

            /*
             *  Metafunction class equivalent to std::identity
             */
            template<class T>
            struct identity {
                template<class...>
                struct fn : type_wrap<T> {};
            };

            /*
             *  Metafunction class equivalent to std::conjunction
             */
            template<class... TraitFnCls>
            struct conjunction {
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
                template<class... Args>
                struct fn : polyfill::conjunction<invoke_t<TraitFnCls, Args...>...> {};
#else
                template<class... Args>
                struct fn : polyfill::conjunction<typename TraitFnCls::template fn<Args...>...> {};
#endif
            };

            /*
             *  Metafunction class equivalent to std::disjunction
             */
            template<class... TraitFnCls>
            struct disjunction {
#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
                template<class... Args>
                struct fn : polyfill::disjunction<invoke_t<TraitFnCls, Args...>...> {};
#else
                template<class... Args>
                struct fn : polyfill::disjunction<typename TraitFnCls::template fn<Args...>...> {};
#endif
            };

            /*
             *  Bind arguments to the front of a metafunction.
             */
            template<template<class...> class Fn, class... Bound>
            using bind_front_fn = bind_front<quote_fn<Fn>, Bound...>;

            /*
             *  Bind arguments to the back of a metafunction.
             */
            template<template<class...> class Fn, class... Bound>
            using bind_back_fn = bind_back<quote_fn<Fn>, Bound...>;

            /*
             *  Bind a metafunction at the front of a higher-order metafunction
             */
            template<template<template<class...> class Fn, class... Args2> class HigherFn,
                     template<class...>
                     class BoundFn>
            using bind_front_higherorder_fn = bind_front<quote_higherorder_front_fn<HigherFn>, quote_fn<BoundFn>>;

            /*
             *  Bind a metafunction at the back of a higher-order metafunction having 2 arguments
             */
            template<template<class Arg1, template<class...> class Fn> class HigherFn, template<class...> class BoundFn>
            using bind_back_2_higherorder_fn = bind_back<quote_2_higherorder_back_fn<HigherFn>, quote_fn<BoundFn>>;

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
            /*
             *  Metafunction equivalent to std::conjunction
             */
            template<template<class...> class... TraitFn>
            using conjunction_fn = conjunction<quote_fn<TraitFn>...>;

            /*
             *  Metafunction equivalent to std::disjunction
             */
            template<template<class...> class... TraitFn>
            using disjunction_fn = disjunction<quote_fn<TraitFn>...>;
#else
            template<template<class...> class... TraitFn>
            struct conjunction_fn : conjunction<quote_fn<TraitFn>...> {};

            template<template<class...> class... TraitFn>
            struct disjunction_fn : disjunction<quote_fn<TraitFn>...> {};
#endif
        }
    }

    namespace mpl = internal::mpl;
}
