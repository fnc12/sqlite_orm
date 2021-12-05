#pragma once

#include <string>  //  std::string
#include <sqlite3.h>
#include <tuple>  //  std::tuple
#include <functional>  //  std::function
#include <algorithm>  //  std::min

namespace sqlite_orm {

    struct arg_values;

    template<class T, class P>
    struct pointer_arg;
    template<class T, class P, class D>
    class pointer_binding;

    namespace internal {

        struct function_base {
            using func_call = std::function<
                void(sqlite3_context *context, void *functionPointer, int argsCount, sqlite3_value **values)>;
            using final_call = std::function<void(sqlite3_context *context, void *functionPointer)>;

            std::string name;
            int argumentsCount = 0;
            std::function<int *()> create;
            void (*destroy)(int *) = nullptr;

            function_base(decltype(name) name_,
                          decltype(argumentsCount) argumentsCount_,
                          decltype(create) create_,
                          decltype(destroy) destroy_) :
                name(move(name_)),
                argumentsCount(argumentsCount_), create(move(create_)), destroy(destroy_) {}
        };

        struct scalar_function_t : function_base {
            func_call run;

            scalar_function_t(decltype(name) name_,
                              int argumentsCount_,
                              decltype(create) create_,
                              decltype(run) run_,
                              decltype(destroy) destroy_) :
                function_base{move(name_), argumentsCount_, move(create_), destroy_},
                run(move(run_)) {}
        };

        struct aggregate_function_t : function_base {
            func_call step;
            final_call finalCall;

            aggregate_function_t(decltype(name) name_,
                                 int argumentsCount_,
                                 decltype(create) create_,
                                 decltype(step) step_,
                                 decltype(finalCall) finalCall_,
                                 decltype(destroy) destroy_) :
                function_base{move(name_), argumentsCount_, move(create_), destroy_},
                step(move(step_)), finalCall(move(finalCall_)) {}
        };

        //  got it from here https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature
        template<class F>
        struct is_scalar_function_impl {

            template<class U, class T>
            struct SFINAE;

            template<class U, class R>
            struct SFINAE<U, R (U::*)() const> {};

            template<class U>
            static char test(SFINAE<U, decltype(&U::operator())> *);

            template<class U>
            static int test(...);

            static constexpr bool has = sizeof(test<F>(0)) == sizeof(char);
        };

        template<class F>
        struct is_aggregate_function_impl {

            template<class U, class T>
            struct SFINAE;

            template<class U, class R>
            struct SFINAE<U, R (U::*)() const> {};

            template<class U>
            static char test(SFINAE<U, decltype(&U::step)> *);

            template<class U>
            static int test(...);

            template<class U>
            static char test2(SFINAE<U, decltype(&U::fin)> *);

            template<class U>
            static int test2(...);

            static constexpr bool has = sizeof(test<F>(0)) == sizeof(char);
            static constexpr bool has2 = sizeof(test2<F>(0)) == sizeof(char);
            static constexpr bool has_both = has && has2;
        };

        template<class F>
        struct is_scalar_function : std::integral_constant<bool, is_scalar_function_impl<F>::has> {};

        template<class F>
        struct is_aggregate_function : std::integral_constant<bool, is_aggregate_function_impl<F>::has_both> {};

        template<class F>
        struct scalar_run_member_pointer {
            using type = decltype(&F::operator());
        };

        template<class F>
        struct aggregate_run_member_pointer {
            using step_type = decltype(&F::step);
            using fin_type = decltype(&F::fin);
        };

        template<class T>
        struct member_function_arguments;

        template<class O, class R, class... Args>
        struct member_function_arguments<R (O::*)(Args...) const> {
            using member_function_type = R (O::*)(Args...) const;
            using tuple_type = std::tuple<typename std::decay<Args>::type...>;
            using return_type = R;
        };

        template<class O, class R, class... Args>
        struct member_function_arguments<R (O::*)(Args...)> {
            using member_function_type = R (O::*)(Args...);
            using tuple_type = std::tuple<typename std::decay<Args>::type...>;
            using return_type = R;
        };

        template<class F, bool IsScalar>
        struct callable_arguments_impl;

        template<class F>
        struct callable_arguments_impl<F, true> {
            using function_member_pointer = typename scalar_run_member_pointer<F>::type;
            using args_tuple = typename member_function_arguments<function_member_pointer>::tuple_type;
            using return_type = typename member_function_arguments<function_member_pointer>::return_type;
        };

        template<class F>
        struct callable_arguments_impl<F, false> {
            using step_function_member_pointer = typename aggregate_run_member_pointer<F>::step_type;
            using fin_function_member_pointer = typename aggregate_run_member_pointer<F>::fin_type;
            using args_tuple = typename member_function_arguments<step_function_member_pointer>::tuple_type;
            using return_type = typename member_function_arguments<fin_function_member_pointer>::return_type;
        };

        template<class F>
        struct callable_arguments : callable_arguments_impl<F, is_scalar_function<F>::value> {};

        template<class F, class... Args>
        struct function_call {
            using function_type = F;
            using args_tuple = std::tuple<Args...>;

            args_tuple args;
        };

        template<typename Type, template<typename...> class Primary>
        SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_v = false;

        template<template<typename...> class Primary, class... Types>
        SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_v<Primary<Types...>, Primary> = true;

        template<typename Type, template<typename...> class Primary>
        struct is_specialization : std::bool_constant<is_specialization_v<Type, Primary>> {};

        template<typename... T>
        using is_specialization_t = typename is_specialization<T...>::type;

        template<typename...>
        SQLITE_ORM_INLINE_VAR constexpr bool always_false_v = false;

        template<size_t I>
        using index_constant = std::integral_constant<size_t, I>;

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

        template<size_t I, const char *PointerArg, const char *Binding>
        SQLITE_ORM_CONSTEVAL bool assert_same_pointer_type() {
            constexpr bool valid = Binding == PointerArg;
            static_assert(valid, "Pointer value types of I-th argument do not match");
            return valid;
        }
        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool expected_pointer_value() {
            static_assert(always_false_v<FnArg, CallArg>, "Expected a pointer value for I-th argument");
            return false;
        }

        template<size_t I, class FnArg, class CallArg, class EnableIfTag = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_same_pvt_v = expected_pointer_value<I, FnArg, CallArg>();

        template<size_t I, class PointerArg, class Binding>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_same_pvt_v<I, PointerArg, Binding, std::void_t<typename PointerArg::tag, typename Binding::tag>> =
                assert_same_pointer_type<I, PointerArg::tag::value, Binding::tag::value>();

        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::false_type) {
            return true;
        }

        template<size_t I, class FnArg, class CallArg>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::true_type) {
            return is_same_pvt_v<I, FnArg, CallArg>;
        }

        template<class FnArgs, class CallArgs>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(index_constant<size_t(-1)>) {
            return true;
        }
        template<class FnArgs, class CallArgs, size_t I>
        SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(index_constant<I>) {
            using func_arg_t = std::tuple_element_t<I, FnArgs>;
            using passed_arg_t = unpacked_arg_t<std::tuple_element_t<I, CallArgs>>;

            constexpr bool valid = validate_pointer_value_type<I,
                                                               std::tuple_element_t<I, FnArgs>,
                                                               unpacked_arg_t<std::tuple_element_t<I, CallArgs>>>(
                std::integral_constant < bool,
                is_specialization_v<func_arg_t, pointer_arg> || is_specialization_v < passed_arg_t,
                pointer_binding >> {});

            return validate_pointer_value_types<FnArgs, CallArgs>(index_constant<I - 1>{}) && valid;
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
                           internal::index_constant<std::min<>(functionArgsCount, argsCount) - 1>{})) ||
                          std::is_same<function_args_tuple, std::tuple<arg_values>>::value,
                      "Number of arguments does not match");
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

}
