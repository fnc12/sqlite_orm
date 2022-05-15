#pragma once

/*
 *  Symbols for 'template metaprogramming' (compile-time template programming),
 *  inspired by the MPL of Aleksey Gurtovoy and David Abrahams.
 *  
 *  Currently, the focus is on facilitating advanced type filtering,
 *  such as filtering columns by constraints having various traits.
 *  Hence it contains only a very small subset of a full MPL.
 *  
 *  Two key concepts are critical to understanding:
 *  1. A 'metafunction' is a class template that represents a function invocable at compile-time.
 *  2. A 'metafunction class' is a certain form of metafunction representation that enables higher-order metaprogramming.
 *     More precisely, it's a class with a nested metafunction called "fn"
 *     Correspondingly, a metafunction class invocation is defined as invocation of its nested "fn" metafunction.
 *  
 *  Conventions:
 *  - "Fn" is the name for a metafunction template template parameter.
 *  - "FnCls" is the name for a metafunction class template parameter.
 *  - "_fn" is a suffix for a type that accepts metafunctions and turns them into metafunction classes.
 *  - "higher order" denotes a metafunction that operates on another metafunction (i.e. takes it as an argument).
 */

#include <type_traits>  //  std::false_type, std::true_type

#include "cxx_universal.h"
#include "cxx_polyfill.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            template<template<class...> class Fn>
            struct indirectly_test_metafunction;

            template<class T, class SFINAE = void>
            struct is_metafunction_class : std::false_type {};
            template<class FnCls>
            struct is_metafunction_class<FnCls, polyfill::void_t<indirectly_test_metafunction<typename FnCls::fn>>>
                : std::true_type {};

            template<class T>
            SQLITE_ORM_INLINE_VAR constexpr bool is_metafunction_class_v = is_metafunction_class<T>::value;

            /*
             *  Operation to access a metafunction class's nested metafunction.
             */
            template<class FnCls>
            using fn_t = typename FnCls::template fn;

            /*
             *  Invoke metafunction.
             */
            template<template<class...> class Fn, class... Args>
            using invoke_fn_t = typename Fn<Args...>::type;

            /*
             *  Invoke metafunction class by invoking its nested metafunction.
             */
            template<class FnCls, class... Args>
            using invoke_t = typename FnCls::template fn<Args...>::type;

            /*
             *  Wrap given type such that `typename T::type` is valid.
             */
            template<class T, class SFINAE = void>
            struct type_wrap : polyfill::type_identity<T> {};
            template<class T>
            struct type_wrap<T, polyfill::void_t<typename T::type>> : T {};

            /*
             *  Turn metafunction into a metafunction class.
             *  
             *  Invocation of the nested metafunction `fn` is SFINAE-friendly (detection idiom).
             *  This is necessary because `fn` is a proxy to the originally quoted metafunction,
             *  and the instantiation of the metafunction might be an invalid expression.
             */
            template<template<class...> class Fn>
            struct quote_fn {
                template<class InvocableTest, template<class...> class Fn, class... Args>
                struct invoke_fn;

                template<template<class...> class Fn, class... Args>
                struct invoke_fn<polyfill::void_t<Fn<Args...>>, Fn, Args...> {
                    using type = type_wrap<Fn<Args...>>;
                };

                template<class... Args>
                using fn = typename invoke_fn<void, Fn, Args...>::type;
            };

            /*
             *  Turn higher-order metafunction into a metafunction class.
             */
            template<template<template<class...> class Fn, class... Args2> class HigherFn>
            struct quote_higherorder_front_fn {
                template<class QuotedFn, class... Args2>
                struct fn : HigherFn<typename QuotedFn::template fn, Args2...> {};
            };

            /*
             *  Metafunction class that extracts a the nested metafunction of its metafunction class argument,
             *  quotes the extracted metafunction and passes it on to the next metafunction class
             *  (kind of the inverse of quoting).
             */
            template<class FnCls>
            struct pass_extracted_fn_to {
                template<class... Args>
                struct fn : FnCls::template fn<Args...> {};

                // extract, quote, pass on
                template<template<class...> class Fn, class... Args>
                struct fn<Fn<Args...>> : FnCls::template fn<quote_fn<Fn>> {};
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
             *  Metafunction class equivalent to polyfill::always_false.
             *  It ignores arguments passed to the metafunction,
             *  and always returns the given type.
             */
            template<class T>
            struct always {
                template<class...>
                struct fn : type_wrap<T> {};
            };

            /*
             *  Unary metafunction class equivalent to std::type_identity.
             */
            struct identity {
                template<class T>
                struct fn : type_wrap<T> {};
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
                     class BoundFn,
                     class... Bound>
            using bind_front_higherorder_fn =
                bind_front<quote_higherorder_front_fn<HigherFn>, quote_fn<BoundFn>, Bound...>;

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
