#pragma once

#include "cxx_type_traits_polyfill.h"
#include "mpl.h"

namespace sqlite_orm {
    namespace internal {
        /*
         *  Define nested typenames:
         *  - return_type
         *  - arguments_tuple
         *  - signature_type
         */
        template<class F>
        struct function_traits;

        /*
         *  A function's return type
         */
        template<class F>
        using function_return_type_t = typename function_traits<F>::return_type;

        /*
         *  A function's arguments tuple
         */
        template<class F,
                 template<class...> class Tuple,
                 template<class...> class ProjectOp = polyfill::type_identity_t>
        using function_arguments = typename function_traits<F>::template arguments_tuple<Tuple, ProjectOp>;

        /*
         *  A function's signature
         */
        template<class F>
        using function_signature_type_t = typename function_traits<F>::signature_type;

        template<class R, class... Args>
        struct function_traits<R(Args...)> {
            using return_type = R;

            template<template<class...> class Tuple, template<class...> class ProjectOp>
            using arguments_tuple = Tuple<mpl::invoke_fn_t<ProjectOp, Args>...>;

            using signature_type = R(Args...);
        };

        // non-exhaustive partial specializations of `function_traits`

        template<class R, class... Args>
        struct function_traits<R(Args...) const> : function_traits<R(Args...)> {
            using signature_type = R(Args...) const;
        };

#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class R, class... Args>
        struct function_traits<R(Args...) noexcept> : function_traits<R(Args...)> {
            using signature_type = R(Args...) noexcept;
        };

        template<class R, class... Args>
        struct function_traits<R(Args...) const noexcept> : function_traits<R(Args...)> {
            using signature_type = R(Args...) const noexcept;
        };
#endif

        /*
         *  Pick signature of function pointer
         */
        template<class F>
        struct function_traits<F(*)> : function_traits<F> {};

        /*
         *  Pick signature of function reference
         */
        template<class F>
        struct function_traits<F(&)> : function_traits<F> {};

        /*
         *  Pick signature of pointer-to-member function
         */
        template<class F, class O>
        struct function_traits<F O::*> : function_traits<F> {};
    }
}
