#pragma once

/*
 *  Symbols for 'template metaprogramming' (compile-time template programming),
 *  inspired by the MPL of Aleksey Gurtovoy and David Abrahams, and the Mp11 of Peter Dimov and Bjorn Reese.
 *  
 *  Currently, the focus is on facilitating advanced type filtering,
 *  such as filtering columns by constraints having various traits.
 *  Hence it contains only a very small subset of a full MPL.
 *  
 *  Three key concepts are critical to understanding:
 *  1. A 'trait' is a class template with a nested `type` typename.
 *     The term 'trait' might be too narrow or not entirely accurate, however in the STL those class templates are summarized as "Type transformations".
 *     hence being "transformation type traits".
 *     It was the traditional way of transforming types before the arrival of alias templates.
 *     E.g. `template<class T> struct x { using type = T; };`
 *     They are of course still available today, but are rather used as building blocks.
 *  2. A 'metafunction' is an alias template for a class template or a nested template expression, whose instantiation yields a type.
 *     E.g. `template<class T> using alias_op_t = typename x<T>::type`
 *  3. A 'quoted metafunction' (aka 'metafunction class') is a certain form of metafunction representation that enables higher-order metaprogramming.
 *     More precisely, it's a class with a nested metafunction called "fn".
 *     Correspondingly, a quoted metafunction invocation is defined as invocation of its nested "fn" metafunction.
 *
 *  Conventions:
 *  - "Fn" is the name of a template template parameter for a metafunction.
 *  - "Q" is the name of class template parameter for a quoted metafunction.
 *  - "_fn" is a suffix for a class or alias template that accepts metafunctions and turns them into quoted metafunctions.
 *  - "higher order" denotes a metafunction that operates on another metafunction (i.e. takes it as an argument).
 */

#include <type_traits>  //  std::enable_if, std::is_same

#include "cxx_universal.h"
#include "cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {
            template<template<class...> class Fn>
            struct indirectly_test_metafunction;

            /*
             *  Determines whether a class template has a nested metafunction `fn`.
             * 
             *  Implementation note: the technique of specialiazing on the inline variable must come first because
             *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
             */
            template<class T, class SFINAE = void>
            SQLITE_ORM_INLINE_VAR constexpr bool is_quoted_metafuntion_v = false;
            template<class Q>
            SQLITE_ORM_INLINE_VAR constexpr bool
                is_quoted_metafuntion_v<Q, polyfill::void_t<indirectly_test_metafunction<Q::template fn>>> = true;

#ifndef SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE
            template<class T>
            using is_quoted_metafuntion = polyfill::bool_constant<is_quoted_metafuntion_v<T>>;
#else
            template<class T>
            struct is_quoted_metafuntion : polyfill::bool_constant<is_quoted_metafuntion_v<T>> {};
#endif

            /*
             *  The indirection through `defer_fn` works around the language inability
             *  to expand `Args...` into a fixed parameter list of an alias template.
             */
            template<template<class...> class Fn, class... Args>
            struct defer {
                using type = Fn<Args...>;
            };

#ifndef SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE
            /*
             *  Invoke metafunction.
             */
            template<template<class...> class Fn, class... Args>
            using invoke_fn_t = Fn<Args...>;
#else
            /*
             *  Invoke metafunction.
             *  
             *  Note: legacy compilers need an extra layer of indirection, otherwise type replacement may fail
             *  if alias template `Fn` has a dependent expression in it.
             */
            template<template<class...> class Fn, class... Args>
            using invoke_fn_t = typename defer<Fn, Args...>::type;
#endif

            /*
             *  Invoke quoted metafunction by invoking its nested metafunction.
             */
            template<class Q, class... Args>
            using invoke_t = typename Q::template fn<Args...>;

            /*
             *  Turn metafunction into a quoted metafunction.
             *  
             *  Invocation of the nested metafunction `fn` is SFINAE-friendly (detection idiom).
             *  This is necessary because `fn` is a proxy to the originally quoted metafunction,
             *  and the instantiation of the metafunction might be an invalid expression.
             */
            template<template<class...> class Fn>
            struct quote_fn {
                template<class InvocableTest, template<class...> class, class...>
                struct invoke_this_fn {};

                template<template<class...> class F, class... Args>
                struct invoke_this_fn<polyfill::void_t<F<Args...>>, F, Args...> {
                    using type = F<Args...>;
                };

                template<class... Args>
                using fn = typename invoke_this_fn<void, Fn, Args...>::type;
            };

            /*
             *  Indirection wrapper for higher-order metafunctions,
             *  specialized on the argument indexes where metafunctions appear.
             */
            template<size_t...>
            struct higherorder;

            template<>
            struct higherorder<0u> {
                /*
                 *  Turn higher-order metafunction into a quoted metafunction.
                 */
                template<template<template<class...> class Fn, class... Args2> class HigherFn>
                struct quote_fn {
                    template<class Q, class... Args>
                    using fn = HigherFn<Q::template fn, Args...>;
                };
            };

            /*
             *  Quoted metafunction that extracts the nested metafunction of its quoted metafunction argument,
             *  quotes the extracted metafunction and passes it on to the next quoted metafunction
             *  (kind of the inverse of quoting).
             */
            template<class Q>
            struct pass_extracted_fn_to {
                template<class... Args>
                struct invoke_this_fn {
                    using type = typename Q::template fn<Args...>;
                };

                // extract class template, quote, pass on
                template<template<class...> class Fn, class... T>
                struct invoke_this_fn<Fn<T...>> {
                    using type = typename Q::template fn<quote_fn<Fn>>;
                };

                template<class... Args>
                using fn = typename invoke_this_fn<Args...>::type;
            };

            /*
             *  Quoted metafunction that invokes the specified metafunctions,
             *  and passes its result on to the next quoted metafunction.
             */
            template<class Q, template<class...> class... Fn>
            struct pass_result_of {
                // invoke `Fn`, pass on their result
                template<class... Args>
                using fn = typename Q::template fn<typename defer<Fn, Args...>::type...>;
            };

            /*
             *  Bind arguments at the front of a quoted metafunction.
             */
            template<class Q, class... Bound>
            struct bind_front {
                template<class... Args>
                using fn = typename Q::template fn<Bound..., Args...>;
            };

            /*
             *  Bind arguments at the back of a quoted metafunction.
             */
            template<class Q, class... Bound>
            struct bind_back {
                template<class... Args>
                using fn = typename Q::template fn<Args..., Bound...>;
            };

            /*
             *  Quoted metafunction equivalent to `polyfill::always_false`.
             *  It ignores arguments passed to the metafunction, and always returns the specified type.
             */
            template<class T>
            struct always {
                template<class... /*Args*/>
                using fn = T;
            };

            /*
             *  Unary quoted metafunction equivalent to `std::type_identity_t`.
             */
            using identity = quote_fn<polyfill::type_identity_t>;

            /*
             *  Quoted metafunction equivalent to `std::negation`.
             */
            template<class TraitQ>
            using not_ = pass_result_of<quote_fn<polyfill::negation>, TraitQ::template fn>;

            /*
             *  Quoted metafunction equivalent to `std::conjunction`.
             */
            template<class... TraitQ>
            using conjunction = pass_result_of<quote_fn<polyfill::conjunction>, TraitQ::template fn...>;

            /*
             *  Quoted metafunction equivalent to `std::disjunction`.
             */
            template<class... TraitQ>
            using disjunction = pass_result_of<quote_fn<polyfill::disjunction>, TraitQ::template fn...>;

            /*
             *  Metafunction equivalent to `std::conjunction`.
             */
            template<template<class...> class... TraitFn>
            using conjunction_fn = pass_result_of<quote_fn<polyfill::conjunction>, TraitFn...>;

            /*
             *  Metafunction equivalent to `std::disjunction`.
             */
            template<template<class...> class... TraitFn>
            using disjunction_fn = pass_result_of<quote_fn<polyfill::disjunction>, TraitFn...>;

            /*
             *  Metafunction equivalent to `std::negation`.
             */
            template<template<class...> class Fn>
            using not_fn = not_<quote_fn<Fn>>;

            /*
             *  Bind arguments at the front of a metafunction.
             */
            template<template<class...> class Fn, class... Bound>
            using bind_front_fn = bind_front<quote_fn<Fn>, Bound...>;

            /*
             *  Bind arguments at the back of a metafunction.
             */
            template<template<class...> class Fn, class... Bound>
            using bind_back_fn = bind_back<quote_fn<Fn>, Bound...>;

            /*
             *  Bind a metafunction and arguments at the front of a higher-order metafunction.
             */
            template<template<template<class...> class Fn, class... Args2> class HigherFn,
                     template<class...>
                     class BoundFn,
                     class... Bound>
            using bind_front_higherorder_fn =
                bind_front<higherorder<0>::quote_fn<HigherFn>, quote_fn<BoundFn>, Bound...>;
        }
    }

    namespace mpl = internal::mpl;

    // convenience quoted metafunctions
    namespace internal {
        /*
         *  Quoted trait metafunction that checks if a type has the specified trait.
         */
        template<template<class...> class TraitFn>
        using check_if = mpl::quote_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks if a type doesn't have the specified trait.
         */
        template<template<class...> class TraitFn>
        using check_if_not = mpl::not_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks if a type (possibly projected) is the same as the specified type.
         *  
         *  `ProjOp` is a projection applied to `Type` and must be a metafunction.
         */
        template<class Type, template<class...> class ProjOp = polyfill::type_identity_t>
        using check_if_is_type = mpl::pass_result_of<mpl::bind_front_fn<std::is_same, Type>, ProjOp>;

        /*
         *  Quoted trait metafunction that checks if a type's template matches the specified template
         *  (similar to `is_specialization_of`).
         */
        template<template<class...> class Template>
        using check_if_is_template =
            mpl::pass_extracted_fn_to<mpl::bind_front_fn<std::is_same, mpl::quote_fn<Template>>>;
    }
}
