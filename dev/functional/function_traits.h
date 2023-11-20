#pragma once
#include "cxx_type_traits_polyfill.h"
#include "mpl.h"

namespace sqlite_orm {
    namespace internal {
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
                 template<class...>
                 class Tuple,
                 template<class...> class ProjectOp = polyfill::type_identity_t>
        using function_arguments = typename function_traits<F>::template arguments_tuple<Tuple, ProjectOp>;

        /*
         *  Define nested typenames:
         *  - return_type
         *  - arguments_tuple
         */
        template<class R, class... Args>
        struct function_traits<R(Args...)> {
            using return_type = R;

            template<template<class...> class Tuple, template<class...> class ProjectOp>
            using arguments_tuple = Tuple<mpl::invoke_fn_t<ProjectOp, Args>...>;
        };

        // non-exhaustive partial specializations of `function_traits`

        template<class R, class... Args>
        struct function_traits<R(Args...) const> : function_traits<R(Args...)> {};

#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class R, class... Args>
        struct function_traits<R(Args...) noexcept> : function_traits<R(Args...)> {};

        template<class R, class... Args>
        struct function_traits<R(Args...) const noexcept> : function_traits<R(Args...)> {};
#endif

        /*
         *  Pick signature of function pointer
         */
        template<class R, class... Args>
        struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {};

        /*
         *  Pick signature of pointer-to-member function
         */
        template<class F, class O>
        struct function_traits<F O::*> : function_traits<F> {};
    }
}
