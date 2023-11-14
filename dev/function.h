#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::is_member_function_pointer, std::remove_const, std::decay, std::is_same, std::false_type, std::true_type
#include <string>  //  std::string
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
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

        struct udf_proxy_base {
            using func_call = std::function<
                void(sqlite3_context* context, void* functionPointer, int argsCount, sqlite3_value** values)>;
            using final_call = std::function<void(sqlite3_context* context, void* functionPointer)>;

            std::string name;
            int argumentsCount = 0;
            std::function<int*()> create;
            void (*destroy)(int*) = nullptr;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            udf_proxy_base(decltype(name) name_,
                           decltype(argumentsCount) argumentsCount_,
                           decltype(create) create_,
                           decltype(destroy) destroy_) :
                name(std::move(name_)),
                argumentsCount(argumentsCount_), create(std::move(create_)), destroy(destroy_) {}
#endif
        };

        struct scalar_udf_proxy : udf_proxy_base {
            func_call run;

            scalar_udf_proxy(decltype(name) name_,
                             int argumentsCount_,
                             decltype(create) create_,
                             decltype(run) run_,
                             decltype(destroy) destroy_) :
                udf_proxy_base{std::move(name_), argumentsCount_, std::move(create_), destroy_},
                run(std::move(run_)) {}
        };

        struct aggregate_udf_proxy : udf_proxy_base {
            func_call step;
            final_call finalCall;

            aggregate_udf_proxy(decltype(name) name_,
                                int argumentsCount_,
                                decltype(create) create_,
                                decltype(step) step_,
                                decltype(finalCall) finalCall_,
                                decltype(destroy) destroy_) :
                udf_proxy_base{std::move(name_), argumentsCount_, std::move(create_), destroy_},
                step(std::move(step_)), finalCall(std::move(finalCall_)) {}
        };

        template<class F>
        using scalar_call_function_t = decltype(&F::operator());

        template<class F>
        using aggregate_step_function_t = decltype(&F::step);

        template<class F>
        using aggregate_fin_function_t = decltype(&F::fin);

        template<class F, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_scalar_udf_v = false;
        template<class F>
        SQLITE_ORM_INLINE_VAR constexpr bool is_scalar_udf_v<F, polyfill::void_t<scalar_call_function_t<F>>> = true;

        template<class F, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_aggregate_udf_v = false;
        template<class F>
        SQLITE_ORM_INLINE_VAR constexpr bool is_aggregate_udf_v<
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
        struct callable_arguments_impl<F, std::enable_if_t<is_scalar_udf_v<F>>> {
            using args_tuple = typename member_function_arguments<scalar_call_function_t<F>>::tuple_type;
            using return_type = typename member_function_arguments<scalar_call_function_t<F>>::return_type;
        };

        template<class F>
        struct callable_arguments_impl<F, std::enable_if_t<is_aggregate_udf_v<F>>> {
            using args_tuple = typename member_function_arguments<aggregate_step_function_t<F>>::tuple_type;
            using return_type = typename member_function_arguments<aggregate_fin_function_t<F>>::return_type;
        };

        template<class F>
        struct callable_arguments : callable_arguments_impl<F> {};

        template<class UDF, class... Args>
        struct function_call {
            using udf_type = UDF;
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

#if __cplusplus >= 201703L  // C++17 or later
        template<size_t I, const char* PointerArg, const char* Binding>
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

        /*
         *  Generator of a user-defined function call in a sql query expression.
         *  Use the variable template `func<>` to instantiate.
         *  Calling the function captures the parameters in a `function_call` node.
         */
        template<class UDF>
        struct function : polyfill::type_identity<UDF> {
            template<typename... Args>
            function_call<UDF, Args...> operator()(Args... args) const {
                using args_tuple = std::tuple<Args...>;
                using function_args_tuple = typename callable_arguments<UDF>::args_tuple;
                constexpr size_t argsCount = std::tuple_size<args_tuple>::value;
                constexpr size_t functionArgsCount = std::tuple_size<function_args_tuple>::value;
                static_assert((argsCount == functionArgsCount &&
                               !std::is_same<function_args_tuple, std::tuple<arg_values>>::value &&
                               validate_pointer_value_types<function_args_tuple, args_tuple>(
                                   polyfill::index_constant<std::min(functionArgsCount, argsCount) - 1>{})) ||
                                  std::is_same<function_args_tuple, std::tuple<arg_values>>::value,
                              "The number of arguments does not match");
                return {{std::forward<Args>(args)...}};
            }
        };
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /** @short Specifies that a type is a user-defined scalar function.
     */
    template<class UDF>
    concept orm_scalar_udf = requires {
        UDF::name();
        typename internal::scalar_call_function_t<UDF>;
    };

    /** @short Specifies that a type is a user-defined aggregate function.
     */
    template<class UDF>
    concept orm_aggregate_udf = requires {
        UDF::name();
        typename internal::aggregate_step_function_t<UDF>;
        typename internal::aggregate_fin_function_t<UDF>;
        requires std::is_member_function_pointer_v<internal::aggregate_step_function_t<UDF>>;
        requires std::is_member_function_pointer_v<internal::aggregate_fin_function_t<UDF>>;
    };

    /** @short Specifies that a type is a framed user-defined scalar function.
     */
    template<class F>
    concept orm_scalar_function = (polyfill::is_specialization_of_v<std::remove_const_t<F>, internal::function> &&
                                   orm_scalar_udf<typename F::type>);

    /** @short Specifies that a type is a framed user-defined aggregate function.
     */
    template<class F>
    concept orm_aggregate_function = (polyfill::is_specialization_of_v<std::remove_const_t<F>, internal::function> &&
                                      orm_aggregate_udf<typename F::type>);
#endif

    /**
     *  Call a user-defined function.
     *  
     *  Example:
     *  struct IdFunc { int oeprator(int arg)() const { return arg; } };
     *  // inline:
     *  select(func<IdFunc>(42));
     *  // As this is a variable template, you can frame the user-defined function and define a variable for syntactic sugar and legibility:
     *  inline constexpr auto idfunc = func<IdFunc>;
     *  select(idfunc(42));
     *  
     */
    template<class UDF>
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        requires(orm_scalar_udf<UDF> || orm_aggregate_udf<UDF>)
#endif
    SQLITE_ORM_INLINE_VAR constexpr internal::function<UDF> func{};
}
