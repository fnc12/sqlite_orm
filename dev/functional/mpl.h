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

#ifndef _IMPORT_STD_MODULE
#include <type_traits>  //  std::true_type, std::false_type, std::is_same, std::negation, std::conjunction, std::disjunction
#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
#include <initializer_list>
#else
#include <array>
#endif
#endif

#include "cxx_type_traits_polyfill.h"
#include "mpl/conditional.h"

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

            template<class T>
            struct is_quoted_metafuntion : polyfill::bool_constant<is_quoted_metafuntion_v<T>> {};

            /*  
             *  Type pack.
             */
            template<class...>
            struct pack {};

            /*
             *  The indirection through `defer_fn` works around the language inability
             *  to expand `Args...` into a fixed parameter list of an alias template.
             *  
             *  Also, legacy compilers need an extra layer of indirection, otherwise type replacement may fail
             *  if alias template `Fn` has a dependent expression in it.
             */
            template<template<class...> class Fn, class... Args>
            struct defer_fn {
                using type = Fn<Args...>;
            };

            /*
             *  The indirection through `defer` works around the language inability
             *  to expand `Args...` into a fixed parameter list of an alias template.
             */
            template<class Q, class... Args>
            struct defer {
                using type = typename Q::template fn<Args...>;
            };

            /*
             *  Invoke metafunction.
             */
            template<template<class...> class Fn, class... Args>
            using invoke_fn_t = typename defer_fn<Fn, Args...>::type;

            /*
             *  Invoke quoted metafunction by invoking its nested metafunction.
             */
            template<class Q, class... Args>
            using invoke_t = typename defer<Q, Args...>::type;

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
                struct invoke_this_fn {
                    // error N: 'type': is not a member of any direct or indirect base class of 'quote_fn<Fn>::invoke_this_fn<void,Fn,T>'
                    // means that the metafunction cannot be called with the passed arguments.
                };

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
                template<template<template<class...> class Fn, class... Args2> class HigherFn, class Q, class... Args>
                struct defer_higher_fn {
                    using type = HigherFn<Q::template fn, Args...>;
                };

                /*
                 *  Turn higher-order metafunction into a quoted metafunction.
                 */
                template<template<template<class...> class Fn, class... Args2> class HigherFn>
                struct quote_fn {
                    template<class Q, class... Args>
                    using fn = typename defer_higher_fn<HigherFn, Q, Args...>::type;
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
             *  Quoted metafunction that invokes the specified quoted metafunctions,
             *  and passes their results on to the next quoted metafunction.
             */
            template<class Q, class... Qs>
            struct pass_result_of {
                // invoke `Fn`, pass on their result
                template<class... Args>
                using fn = typename Q::template fn<typename defer<Qs, Args...>::type...>;
            };

            /*
             *  Quoted metafunction that invokes the specified metafunctions,
             *  and passes their results on to the next quoted metafunction.
             */
            template<class Q, template<class...> class... Fn>
            using pass_result_of_fn = pass_result_of<Q, quote_fn<Fn>...>;

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
            using not_ = pass_result_of<quote_fn<polyfill::negation>, TraitQ>;

            /*
             *  Quoted metafunction equivalent to `std::conjunction`.
             */
            template<class... TraitQ>
            struct conjunction {
                template<class... Args>
                using fn = std::true_type;
            };

            template<class FirstQ, class... TraitQ>
            struct conjunction<FirstQ, TraitQ...> {
                // match last or `std::false_type`
                template<class ArgPack, class ResultTrait, class...>
                struct invoke_this_fn {
                    static_assert(std::is_same<ResultTrait, std::true_type>::value ||
                                      std::is_same<ResultTrait, std::false_type>::value,
                                  "Resulting trait must be a std::bool_constant");
                    using type = ResultTrait;
                };

                // match `std::true_type` and one or more remaining
                template<class... Args, class NextQ, class... RestQ>
                struct invoke_this_fn<pack<Args...>, std::true_type, NextQ, RestQ...>
                    : invoke_this_fn<pack<Args...>,
                                     // access resulting trait::type
                                     typename defer<NextQ, Args...>::type::type,
                                     RestQ...> {};

                template<class... Args>
                using fn = typename invoke_this_fn<pack<Args...>,
                                                   // access resulting trait::type
                                                   typename defer<FirstQ, Args...>::type::type,
                                                   TraitQ...>::type;
            };

            /*
             *  Quoted metafunction equivalent to `std::disjunction`.
             */
            template<class... TraitQ>
            struct disjunction {
                template<class... Args>
                using fn = std::false_type;
            };

            template<class FirstQ, class... TraitQ>
            struct disjunction<FirstQ, TraitQ...> {
                // match last or `std::true_type`
                template<class ArgPack, class ResultTrait, class...>
                struct invoke_this_fn {
                    static_assert(std::is_same<ResultTrait, std::true_type>::value ||
                                      std::is_same<ResultTrait, std::false_type>::value,
                                  "Resulting trait must be a std::bool_constant");
                    using type = ResultTrait;
                };

                // match `std::false_type` and one or more remaining
                template<class... Args, class NextQ, class... RestQ>
                struct invoke_this_fn<pack<Args...>, std::false_type, NextQ, RestQ...>
                    : invoke_this_fn<pack<Args...>,
                                     // access resulting trait::type
                                     typename defer<NextQ, Args...>::type::type,
                                     RestQ...> {};

                template<class... Args>
                using fn = typename invoke_this_fn<pack<Args...>,
                                                   // access resulting trait::type
                                                   typename defer<FirstQ, Args...>::type::type,
                                                   TraitQ...>::type;
            };

            /*
             *  Metafunction equivalent to `std::conjunction`.
             */
            template<template<class...> class... TraitFn>
            using conjunction_fn = pass_result_of_fn<quote_fn<polyfill::conjunction>, TraitFn...>;

            /*
             *  Metafunction equivalent to `std::disjunction`.
             */
            template<template<class...> class... TraitFn>
            using disjunction_fn = pass_result_of_fn<quote_fn<polyfill::disjunction>, TraitFn...>;

            /*
             *  Metafunction equivalent to `std::negation`.
             */
            template<template<class...> class Fn>
            using not_fn = pass_result_of_fn<quote_fn<polyfill::negation>, Fn>;

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
                     template<class...> class BoundFn,
                     class... Bound>
            using bind_front_higherorder_fn =
                bind_front<higherorder<0>::quote_fn<HigherFn>, quote_fn<BoundFn>, Bound...>;

#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
            constexpr size_t find_first_true_helper(std::initializer_list<bool> values) {
                size_t i = 0;
                for (auto first = values.begin(); first != values.end() && !*first; ++first) {
                    ++i;
                }
                return i;
            }

            constexpr size_t count_true_helper(std::initializer_list<bool> values) {
                size_t n = 0;
                for (auto first = values.begin(); first != values.end(); ++first) {
                    n += *first;
                }
                return n;
            }
#else
            template<size_t N>
            constexpr size_t find_first_true_helper(const std::array<bool, N>& values, size_t i = 0) {
                return i == N || values[i] ? 0 : 1 + find_first_true_helper(values, i + 1);
            }

            template<size_t N>
            constexpr size_t count_true_helper(const std::array<bool, N>& values, size_t i = 0) {
                return i == N ? 0 : values[i] + count_true_helper(values, i + 1);
            }
#endif

            /*
             *  Quoted metafunction that invokes the specified quoted predicate metafunction on each element of a type list,
             *  and returns the index constant of the first element for which the predicate returns true.
             */
            template<class PredicateQ>
            struct finds {
                template<class Pack, class ProjectQ>
                struct invoke_this_fn {
                    static_assert(polyfill::always_false_v<Pack>,
                                  "`finds` must be invoked with a type list as first argument.");
                };

                template<template<class...> class Pack, class... T, class ProjectQ>
                struct invoke_this_fn<Pack<T...>, ProjectQ> {
                    // hoist result into `value` [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
                    static constexpr size_t value = find_first_true_helper
#ifndef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
                        <sizeof...(T)>
#endif
                        ({PredicateQ::template fn<typename ProjectQ::template fn<T>>::value...});
                    using type = polyfill::index_constant<value>;
                };

                template<class Pack, class ProjectQ = identity>
                using fn = typename invoke_this_fn<Pack, ProjectQ>::type;
            };

            template<template<class...> class PredicateFn>
            using finds_fn = finds<quote_fn<PredicateFn>>;

            /*
             *  Quoted metafunction that invokes the specified quoted predicate metafunction on each element of a type list,
             *  and returns the index constant of the first element for which the predicate returns true.
             */
            template<class PredicateQ>
            struct counts {
                template<class Pack, class ProjectQ>
                struct invoke_this_fn {
                    static_assert(polyfill::always_false_v<Pack>,
                                  "`counts` must be invoked with a type list as first argument.");
                };

                template<template<class...> class Pack, class... T, class ProjectQ>
                struct invoke_this_fn<Pack<T...>, ProjectQ> {
                    // hoist result into `value` [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
                    static constexpr size_t value = count_true_helper
#ifndef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
                        <sizeof...(T)>
#endif
                        ({PredicateQ::template fn<typename ProjectQ::template fn<T>>::value...});
                    using type = polyfill::index_constant<value>;
                };

                template<class Pack, class ProjectQ = identity>
                using fn = typename invoke_this_fn<Pack, ProjectQ>::type;
            };

            template<template<class...> class PredicateFn>
            using counts_fn = counts<quote_fn<PredicateFn>>;

            /*
             *  Quoted metafunction that invokes the specified quoted predicate metafunction on each element of a type list,
             *  and returns the index constant of the first element for which the predicate returns true.
             */
            template<class TraitQ>
            struct contains {
                template<class Pack, class ProjectQ>
                struct invoke_this_fn {
                    static_assert(polyfill::always_false_v<Pack>,
                                  "`contains` must be invoked with a type list as first argument.");
                };

                template<template<class...> class Pack, class... T, class ProjectQ>
                struct invoke_this_fn<Pack<T...>, ProjectQ> {
                    // hoist result into `value` [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
                    static constexpr size_t value =
                        static_cast<bool>(count_true_helper
#ifndef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
                                          <sizeof...(T)>
#endif
                                          ({TraitQ::template fn<typename ProjectQ::template fn<T>>::value...}));
                    using type = polyfill::bool_constant<value>;
                };

                template<class Pack, class ProjectQ = identity>
                using fn = typename invoke_this_fn<Pack, ProjectQ>::type;
            };

            template<template<class...> class TraitFn>
            using contains_fn = contains<quote_fn<TraitFn>>;
        }
    }

    namespace mpl = internal::mpl;

    // convenience quoted metafunctions
    namespace internal {
        /*
         *  Quoted trait metafunction that checks if a type has the specified trait.
         */
        template<template<class...> class TraitFn, class... Bound>
        using check_if =
            mpl::conditional_t<sizeof...(Bound) == 0, mpl::quote_fn<TraitFn>, mpl::bind_front_fn<TraitFn, Bound...>>;

        /*
         *  Quoted trait metafunction that checks if a type doesn't have the specified trait.
         */
        template<template<class...> class TraitFn>
        using check_if_not = mpl::not_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks if a type is the same as the specified type.
         *  Commonly used named abbreviation for `check_if<std::is_same, Type>`.
         */
        template<class Type>
        using check_if_is_type = mpl::bind_front_fn<std::is_same, Type>;

        /*
         *  Quoted trait metafunction that checks if a type's template matches the specified template
         *  (similar to `is_specialization_of`).
         */
        template<template<class...> class Template>
        using check_if_is_template =
            mpl::pass_extracted_fn_to<mpl::bind_front_fn<std::is_same, mpl::quote_fn<Template>>>;

        /*
         *  Quoted metafunction that finds the index of the given type in a tuple.
         */
        template<class Type>
        using finds_if_has_type = mpl::finds<check_if_is_type<Type>>;

        /*
         *  Quoted metafunction that finds the index of the given class template in a tuple.
         */
        template<template<class...> class Template>
        using finds_if_has_template = mpl::finds<check_if_is_template<Template>>;

        /*
         *  Quoted trait metafunction that counts tuple elements having a given trait.
         */
        template<template<class...> class TraitFn>
        using counts_if_has = mpl::counts_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks whether a tuple contains a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_has = mpl::contains_fn<TraitFn>;

        /*
         *  Quoted trait metafunction that checks whether a tuple doesn't contain a type with given trait.
         */
        template<template<class...> class TraitFn>
        using check_if_has_not = mpl::not_<mpl::contains_fn<TraitFn>>;

        /*
         *  Quoted metafunction that checks whether a tuple contains given type.
         */
        template<class T>
        using check_if_has_type = mpl::contains<check_if_is_type<T>>;

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
        using check_if_has_template = mpl::contains<check_if_is_template<Template>>;
    }
}
