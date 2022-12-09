#pragma once

#include <sqlite3.h>
#include <type_traits>
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <functional>  //  std::function
#include <algorithm>  //  std::min
#include <utility>  //  std::move, std::forward

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    struct arg_values;

    template<class T, class P>
    struct pointer_arg;
    template<class T, class P, class D>
    class pointer_binding;

    namespace internal {

        struct user_defined_function_base {
            using func_call = std::function<
                void(sqlite3_context *context, void *functionPointer, int argsCount, sqlite3_value **values)>;
            using final_call = std::function<void(sqlite3_context *context, void *functionPointer)>;

            std::string name;
            int argumentsCount = 0;
            std::function<int *()> create;
            void (*destroy)(int *) = nullptr;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            user_defined_function_base(decltype(name) name_,
                                       decltype(argumentsCount) argumentsCount_,
                                       decltype(create) create_,
                                       decltype(destroy) destroy_) :
                name(move(name_)),
                argumentsCount(argumentsCount_), create(move(create_)), destroy(destroy_) {}
#endif
        };

        struct user_defined_scalar_function_t : user_defined_function_base {
            func_call run;

            user_defined_scalar_function_t(decltype(name) name_,
                                           int argumentsCount_,
                                           decltype(create) create_,
                                           decltype(run) run_,
                                           decltype(destroy) destroy_) :
                user_defined_function_base{move(name_), argumentsCount_, move(create_), destroy_},
                run(move(run_)) {}
        };

        struct user_defined_aggregate_function_t : user_defined_function_base {
            func_call step;
            final_call finalCall;

            user_defined_aggregate_function_t(decltype(name) name_,
                                              int argumentsCount_,
                                              decltype(create) create_,
                                              decltype(step) step_,
                                              decltype(finalCall) finalCall_,
                                              decltype(destroy) destroy_) :
                user_defined_function_base{move(name_), argumentsCount_, move(create_), destroy_},
                step(move(step_)), finalCall(move(finalCall_)) {}
        };

        template<class F>
        using scalar_call_function_t = decltype(&F::operator());

        template<class F>
        using aggregate_step_function_t = decltype(&F::step);

        template<class F>
        using aggregate_fin_function_t = decltype(&F::fin);

        template<class F, class = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_scalar_function_v = false;
        template<class F>
        SQLITE_ORM_INLINE_VAR constexpr bool is_scalar_function_v<F, polyfill::void_t<scalar_call_function_t<F>>> =
            true;

        template<class F, class = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_aggregate_function_v = false;
        template<class F>
        SQLITE_ORM_INLINE_VAR constexpr bool is_aggregate_function_v<
            F,
            polyfill::void_t<aggregate_step_function_t<F>,
                             aggregate_fin_function_t<F>,
                             std::enable_if_t<std::is_member_function_pointer<aggregate_step_function_t<F>>::value>,
                             std::enable_if_t<std::is_member_function_pointer<aggregate_fin_function_t<F>>::value>>> =
            true;

        template<class T>
        struct member_function_arguments;

        template<class O, class R, class... Args>
        struct member_function_arguments<R (O::*)(Args...) const> {
            using member_function_type = R (O::*)(Args...) const;
            using tuple_type = std::tuple<std::decay_t<Args>...>;
            using return_type = R;
        };

        template<class O, class R, class... Args>
        struct member_function_arguments<R (O::*)(Args...)> {
            using member_function_type = R (O::*)(Args...);
            using tuple_type = std::tuple<std::decay_t<Args>...>;
            using return_type = R;
        };

        template<class F, class SFINAE = void>
        struct callable_arguments_impl;

        template<class F>
        struct callable_arguments_impl<F, std::enable_if_t<is_scalar_function_v<F>>> {
            using args_tuple = typename member_function_arguments<scalar_call_function_t<F>>::tuple_type;
            using return_type = typename member_function_arguments<scalar_call_function_t<F>>::return_type;
        };

        template<class F>
        struct callable_arguments_impl<F, std::enable_if_t<is_aggregate_function_v<F>>> {
            using args_tuple = typename member_function_arguments<aggregate_step_function_t<F>>::tuple_type;
            using return_type = typename member_function_arguments<aggregate_fin_function_t<F>>::return_type;
        };

        template<class F>
        struct callable_arguments : callable_arguments_impl<F> {};

        template<class F, class... Args>
        struct function_call {
            using function_type = F;
            using args_tuple = std::tuple<Args...>;

            args_tuple args;
        };

        template<class T>
        struct unpacked_arg {
            using type = T;
        };
        template<class F, class... Args>
        struct unpacked_arg<function_call<F, Args...>> {
            using type = typename callable_arguments<F>::return_type;
        };
        template<class T>
        using unpacked_arg_t = typename unpacked_arg<T>::type;

        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool expected_pointer_value() {
            static_assert(polyfill::always_false_v<FnArg, CallArg>, "Expected a pointer value for I-th argument");
            return false;
        }

        template<size_t I, class FnArg, class CallArg, class EnableIfTag = void>
        constexpr bool is_same_pvt_v = expected_pointer_value<I, FnArg, CallArg>();

        // Always allow binding nullptr to a pointer argument
        template<size_t I, class PointerArg>
        constexpr bool is_same_pvt_v<I, PointerArg, nullptr_t, polyfill::void_t<typename PointerArg::tag>> = true;

#if __cplusplus >= 201703L  // using C++17 or higher
        template<size_t I, const char *PointerArg, const char *Binding>
        SQLITE_ORM_CONSTEVAL bool assert_same_pointer_type() {
            constexpr bool valid = Binding == PointerArg;
            static_assert(valid, "Pointer value types of I-th argument do not match");
            return valid;
        }

        template<size_t I, class PointerArg, class Binding>
        constexpr bool
            is_same_pvt_v<I, PointerArg, Binding, polyfill::void_t<typename PointerArg::tag, typename Binding::tag>> =
                assert_same_pointer_type<I, PointerArg::tag::value, Binding::tag::value>();
#else
        template<size_t I, class PointerArg, class Binding>
        SQLITE_ORM_CONSTEVAL bool assert_same_pointer_type() {
            constexpr bool valid = Binding::value == PointerArg::value;
            static_assert(valid, "Pointer value types of I-th argument do not match");
            return valid;
        }

        template<size_t I, class PointerArg, class Binding>
        constexpr bool
            is_same_pvt_v<I, PointerArg, Binding, polyfill::void_t<typename PointerArg::tag, typename Binding::tag>> =
                assert_same_pointer_type<I, typename PointerArg::tag, typename Binding::tag>();
#endif

        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::false_type) {
            return true;
        }

        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::true_type) {
            return is_same_pvt_v<I, FnArg, CallArg>;
        }

        template<class FnArgs, class CallArgs>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(polyfill::index_constant<size_t(-1)>) {
            return true;
        }
        template<class FnArgs, class CallArgs, size_t I>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(polyfill::index_constant<I>) {
            using func_arg_t = std::tuple_element_t<I, FnArgs>;
            using passed_arg_t = unpacked_arg_t<std::tuple_element_t<I, CallArgs>>;

#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
            constexpr bool valid = validate_pointer_value_type<I,
                                                               std::tuple_element_t<I, FnArgs>,
                                                               unpacked_arg_t<std::tuple_element_t<I, CallArgs>>>(
                polyfill::bool_constant < (polyfill::is_specialization_of_v<func_arg_t, pointer_arg>) ||
                (polyfill::is_specialization_of_v<passed_arg_t, pointer_binding>) > {});

            return validate_pointer_value_types<FnArgs, CallArgs>(polyfill::index_constant<I - 1>{}) && valid;
#else
            return validate_pointer_value_types<FnArgs, CallArgs>(polyfill::index_constant<I - 1>{}) &&
                   validate_pointer_value_type<I,
                                               std::tuple_element_t<I, FnArgs>,
                                               unpacked_arg_t<std::tuple_element_t<I, CallArgs>>>(
                       polyfill::bool_constant < (polyfill::is_specialization_of_v<func_arg_t, pointer_arg>) ||
                       (polyfill::is_specialization_of_v<passed_arg_t, pointer_binding>) > {});
#endif
        }
    }

    /**
     *  Used to call user defined function: `func<MyFunc>(...);`
     */
    template<class F, class... Args>
    internal::function_call<F, Args...> func(Args... args) {
        using args_tuple = std::tuple<Args...>;
        using function_args_tuple = typename internal::callable_arguments<F>::args_tuple;
        constexpr auto argsCount = std::tuple_size<args_tuple>::value;
        constexpr auto functionArgsCount = std::tuple_size<function_args_tuple>::value;
        static_assert((argsCount == functionArgsCount &&
                       !std::is_same<function_args_tuple, std::tuple<arg_values>>::value &&
                       internal::validate_pointer_value_types<function_args_tuple, args_tuple>(
                           polyfill::index_constant<std::min<>(functionArgsCount, argsCount) - 1>{})) ||
                          std::is_same<function_args_tuple, std::tuple<arg_values>>::value,
                      "Number of arguments does not match");
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

}
