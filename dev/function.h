#pragma once

#include <type_traits>  //  std::enable_if, std::is_member_function_pointer, std::is_function, std::remove_const, std::remove_pointer, std::decay, std::is_same, std::false_type, std::true_type
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  //  std::copy_constructible
#endif
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#include <algorithm>  //  std::min, std::copy_n
#include <utility>  //  std::move, std::forward

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cstring_literal.h"
#include "functional/function_traits.h"
#include "type_traits.h"
#include "tags.h"

namespace sqlite_orm {

    struct arg_values;

    template<class T, class P>
    struct pointer_arg;
    template<class T, class P, class D>
    class pointer_binding;

    namespace internal {
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

        template<class UDF>
        struct function;
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /** @short Specifies that a type is a function signature (i.e. a function in the C++ type system).
     */
    template<class Sig>
    concept orm_function_sig = std::is_function_v<Sig>;

    /** @short Specifies that a type is a classic function object.
     *  
     *  A classic function object meets the following requirements:
     *  - defines a single call operator `F::operator()`
     *  - isn't a traditional sqlite_orm scalar function (having a static `F::name()` function
     */
    template<class F>
    concept orm_classic_function_object =
        ((!requires { typename F::is_transparent; }) && (requires { &F::operator(); }) &&
         /*rule out sqlite_orm scalar function*/
         (!requires { F::name(); }));

    /** @short Specifies that a type is a user-defined scalar function.
     *  
     *  `UDF` must meet the following requirements:
     *  - `UDF::name()` static function
     *  - `UDF::operator()()` call operator
     */
    template<class UDF>
    concept orm_scalar_udf = requires {
        UDF::name();
        typename internal::scalar_call_function_t<UDF>;
    };

    /** @short Specifies that a type is a user-defined aggregate function.
     *  
     *  `UDF` must meet the following requirements:
     *  - `UDF::name()` static function
     *  - `UDF::step()` member function
     *  - `UDF::fin()` member function
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
                                   orm_scalar_udf<typename F::udf_type>);

    /** @short Specifies that a type is a framed user-defined aggregate function.
     */
    template<class F>
    concept orm_aggregate_function = (polyfill::is_specialization_of_v<std::remove_const_t<F>, internal::function> &&
                                      orm_aggregate_udf<typename F::udf_type>);

    /** @short Specifies that a type is a framed and quoted user-defined scalar function.
     */
    template<class Q>
    concept orm_quoted_scalar_function = requires(const Q& quotedF) {
        quotedF.name();
        quotedF.callable();
    };
#endif

    namespace internal {
        template<class F, class SFINAE = void>
        struct callable_arguments_impl;

        template<class F>
        struct callable_arguments_impl<F, std::enable_if_t<is_scalar_udf_v<F>>> {
            using args_tuple = function_arguments<scalar_call_function_t<F>, std::tuple, std::decay_t>;
            using return_type = function_return_type_t<scalar_call_function_t<F>>;
        };

        template<class F>
        struct callable_arguments_impl<F, std::enable_if_t<is_aggregate_udf_v<F>>> {
            using args_tuple = function_arguments<aggregate_step_function_t<F>, std::tuple, std::decay_t>;
            using return_type = function_return_type_t<aggregate_fin_function_t<F>>;
        };

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<class F>
            requires(std::is_function_v<F>)
        struct callable_arguments_impl<F, void> {
            using args_tuple = function_arguments<F, std::tuple, std::decay_t>;
            using return_type = std::decay_t<function_return_type_t<F>>;
        };
#endif

        template<class F>
        struct callable_arguments : callable_arguments_impl<F> {};

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /*
         *  Bundle of type and name of a quoted user-defined function.
         */
        template<class UDF>
        struct udf_holder : private std::string {
            using udf_type = UDF;

            using std::string::basic_string;

            const std::string& operator()() const {
                return *this;
            }
        };
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /*
         *  Bundle of type and name of a traditional sqlite_orm user-defined function.
         */
        template<class UDF>
            requires(requires { UDF::name(); })
        struct udf_holder<UDF>
#else
        /*
         *  Bundle of type and name of a traditional sqlite_orm user-defined function.
         */
        template<class UDF>
        struct udf_holder
#endif
        {
            using udf_type = UDF;

            template<class R = decltype(UDF::name()),
                     std::enable_if_t<polyfill::negation_v<std::is_same<R, char>>, bool> = true>
            decltype(auto) operator()() const {
                return UDF::name();
            }

            template<class R = decltype(UDF::name()), std::enable_if_t<std::is_same<R, char>::value, bool> = true>
            std::string operator()() const {
                return std::string{UDF::name()};
            }
        };

        /*
         *  Represents a call of a user-defined function.
         */
        template<class UDF, class... CallArgs>
        struct function_call {
            using udf_type = UDF;
            using args_tuple = std::tuple<CallArgs...>;

            udf_holder<udf_type> name;
            args_tuple callArgs;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of_v<T, function_call>>> = true;

        template<class T>
        struct unpacked_arg {
            using type = T;
        };
        template<class F, class... CallArgs>
        struct unpacked_arg<function_call<F, CallArgs...>> {
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

        template<typename UDF, typename... CallArgs>
#ifdef SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
        SQLITE_ORM_CONSTEVAL
#endif
            void
            check_function_call() {
            using args_tuple = std::tuple<CallArgs...>;
            using function_args_tuple = typename callable_arguments<UDF>::args_tuple;
            constexpr size_t argsCount = std::tuple_size<args_tuple>::value;
            constexpr size_t functionArgsCount = std::tuple_size<function_args_tuple>::value;
            static_assert((argsCount == functionArgsCount &&
                           !std::is_same<function_args_tuple, std::tuple<arg_values>>::value &&
                           validate_pointer_value_types<function_args_tuple, args_tuple>(
                               polyfill::index_constant<std::min(functionArgsCount, argsCount) - 1>{})) ||
                              std::is_same<function_args_tuple, std::tuple<arg_values>>::value,
                          "The number of arguments does not match");
        }

        /*
         *  Generator of a user-defined function call in a sql query expression.
         *  
         *  Use the variable template `func<>` to instantiate.
         *  
         *  Calling the function captures the parameters in a `function_call` node.
         */
        template<class UDF>
        struct function {
            using udf_type = UDF;
            using callable_type = UDF;

            /*
             *  Generates the SQL function call.
             */
            template<typename... CallArgs>
            function_call<UDF, CallArgs...> operator()(CallArgs... callArgs) const {
                check_function_call<UDF, CallArgs...>();
                return {this->udf_holder(), {std::forward<CallArgs>(callArgs)...}};
            }

            constexpr auto udf_holder() const {
                return internal::udf_holder<UDF>{};
            }

            // returns a character range
            constexpr auto name() const {
                return this->udf_holder()();
            }
        };

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /*
         *  Generator of a user-defined function call in a sql query expression.
         *  
         *  Use the string literal operator template `""_scalar.quote()` to quote
         *  a freestanding function, stateless lambda or function object.
         *  
         *  Calling the function captures the parameters in a `function_call` node.
         *  
         *  Internal note:
         *  1. Captures and represents a function [pointer or object], especially one without side effects.
         *  If `F` is a stateless function object, `quoted_scalar_function::callable()` returns the original function object,
         *  otherwise it is assumed to have possibe side-effects and `quoted_scalar_function::callable()` returns a copy.
         *  2. The nested `udf_type` typename is deliberately chosen to be the function signature,
         *  and will be the abstracted version of the user-defined function. 
         */
        template<class F, class Sig, size_t N>
        struct quoted_scalar_function {
            using udf_type = Sig;
            using callable_type = F;

            /*
             *  Generates the SQL function call.
             */
            template<typename... CallArgs>
            function_call<udf_type, CallArgs...> operator()(CallArgs... callArgs) const {
                check_function_call<udf_type, CallArgs...>();
                return {this->udf_holder(), {std::forward<CallArgs>(callArgs)...}};
            }

            /*
             *  Return original `udf` if stateless or a copy of it otherwise
             */
            constexpr decltype(auto) callable() const {
                if constexpr(stateless<F>) {
                    return (this->udf);
                } else {
                    // non-const copy
                    return F(this->udf);
                }
            }

            constexpr auto udf_holder() const {
                return internal::udf_holder<udf_type>{this->name()};
            }

            constexpr auto name() const {
                return this->nme;
            }

            template<class... Args>
            consteval quoted_scalar_function(const char (&name)[N], Args&&... constructorArgs) :
                udf(std::forward<Args>(constructorArgs)...) {
                std::copy_n(name, N, this->nme);
            }

            F udf;
            char nme[N];
        };

        template<size_t N>
        struct quoted_function_builder : cstring_literal<N> {
            using cstring_literal<N>::cstring_literal;

            /*
             *  From a freestanding function.
             */
            template<class F>
                requires(std::is_function_v<std::remove_pointer_t<F>>)
            [[nodiscard]] consteval auto quote(F callable) const {
                using Sig = function_signature_type_t<F>;
                return quoted_scalar_function<F, Sig, N>{this->cstr, std::move(callable)};
            }

            /*
             *  From an overloaded freestanding function.
             */
            template<orm_function_sig F>
            [[nodiscard]] consteval auto quote(F* callable) const {
                return quoted_scalar_function<F*, F, N>{this->cstr, std::move(callable)};
            }

            /*
             *  From a classic function object instance.
             */
            template<class F>
                requires(orm_classic_function_object<F> && (stateless<F> || std::copy_constructible<F>))
            [[nodiscard]] consteval auto quote(F callable) const {
                using Sig = function_signature_type_t<decltype(&F::operator())>;
                // detect whether overloaded call operator can be picked using `Sig`
                using call_operator_type = decltype(static_cast<Sig F::*>(&F::operator()));
                return quoted_scalar_function<F, Sig, N>{this->cstr, std::move(callable)};
            }

            /*
             *  From a function object instance, picking the overloaded call operator.
             */
            template<orm_function_sig Sig, class F>
                requires((stateless<F> || std::copy_constructible<F>))
            [[nodiscard]] consteval auto quote(F callable) const {
                // detect whether overloaded call operator can be picked using `Sig`
                using call_operator_type = decltype(static_cast<Sig F::*>(&F::operator()));
                return quoted_scalar_function<F, Sig, N>{this->cstr, std::move(callable)};
            }

            /*
             *  From a classic function object type.
             */
            template<orm_classic_function_object F, class... Args>
                requires(stateless<F> || std::copy_constructible<F>)
            [[nodiscard]] consteval auto quote(Args&&... constructorArgs) const {
                using Sig = function_signature_type_t<decltype(&F::operator())>;
                return quoted_scalar_function<F, Sig, N>{this->cstr, std::forward<Args>(constructorArgs)...};
            }

            /*
             *  From a function object type, picking the overloaded call operator.
             */
            template<orm_function_sig Sig, class F, class... Args>
                requires((stateless<F> || std::copy_constructible<F>))
            [[nodiscard]] consteval auto quote(Args&&... constructorArgs) const {
                // detect whether overloaded call operator can be picked using `Sig`
                using call_operator_type = decltype(static_cast<Sig F::*>(&F::operator()));
                return quoted_scalar_function<F, Sig, N>{this->cstr, std::forward<Args>(constructorArgs)...};
            }
        };
#endif
    }

    /** @short Call a user-defined function.
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

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /*  @short Create a scalar function from a freestanding function, stateless lambda or function object,
     *  and call such a user-defined function.
     *  
     *  If you need to pick a function or method from an overload set, or pick a template function you can
     *  specify an explicit function signature in the call to `from()`.
     *  
     *  Examples:
     *  // freestanding function from a library
     *  constexpr auto clamp_int_f = "clamp_int"_scalar.quote(std::clamp<int>);
     *  // stateless lambda
     *  constexpr auto is_fatal_error_f = "IS_FATAL_ERROR"_scalar.quote([](unsigned long errcode) {
     *      return errcode != 0;
     *  });
     *  // function object instance
     *  constexpr auto equal_to_int_f = "equal_to"_scalar.quote(std::equal_to<int>{});
     *  // function object
     *  constexpr auto equal_to_int_2_f = "equal_to"_scalar.quote<std::equal_to<int>>();
     *  // pick function object's template call operator
     *  constexpr auto equal_to_int_3_f = "equal_to"_scalar.quote<bool(const int&, const int&) const>(std::equal_to<void>{});
     *
     *  storage.create_scalar_function<clamp_int_f>();
     *  storage.create_scalar_function<is_fatal_error_f>();
     *  storage.create_scalar_function<equal_to_int_f>();
     *  storage.create_scalar_function<equal_to_int_2_f>();
     *  storage.create_scalar_function<equal_to_int_3_f>();
     *
     *  auto rows = storage.select(clamp_int_f(0, 1, 1));
     *  auto rows = storage.select(is_fatal_error_f(1));
     *  auto rows = storage.select(equal_to_int_f(1, 1));
     *  auto rows = storage.select(equal_to_int_2_f(1, 1));
     *  auto rows = storage.select(equal_to_int_3_f(1, 1));
     */
    template<internal::quoted_function_builder builder>
    [[nodiscard]] consteval auto operator"" _scalar() {
        return builder;
    }
#endif
}
