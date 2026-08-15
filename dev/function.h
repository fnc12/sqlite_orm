#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_member_function_pointer, std::is_function, std::remove_const, std::decay, std::is_convertible, std::is_same, std::false_type, std::true_type, std::is_pointer
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  //  std::copy_constructible
#endif
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#include <functional>  // std::bind_front
#include <algorithm>  //  std::min, std::copy_n
#include <utility>  //  std::move, std::forward
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/cstring_literal.h"
#include "functional/function_traits.h"
#include "type_traits.h"
#include "vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

// export forward-declarations
SQLITE_ORM_EXPORT namespace sqlite_orm {
    struct arg_values;

    // note (internal): forward declare even if `SQLITE_VERSION_NUMBER < 3020000` in order to simplify coding below
    template<class P, class T>
    struct pointer_arg;
    // note (internal): forward declare even if `SQLITE_VERSION_NUMBER < 3020000` in order to simplify coding below
    template<class P, class T, class D>
    class pointer_binding;
}

namespace sqlite_orm::internal {
    template<class F>
    using scalar_call_function_t = decltype(&F::operator());

    template<class F>
    using aggregate_step_function_t = decltype(&F::step);

    template<class F>
    using aggregate_fin_function_t = decltype(&F::fin);

    template<class F>
    constexpr bool is_scalar_udf_v<F, polyfill::void_t<scalar_call_function_t<F>>> = true;

    template<class F>
    constexpr bool is_aggregate_udf_v<
        F,
        polyfill::void_t<aggregate_step_function_t<F>,
                         aggregate_fin_function_t<F>,
                         std::enable_if_t<std::is_member_function_pointer<aggregate_step_function_t<F>>::value>,
                         std::enable_if_t<std::is_member_function_pointer<aggregate_fin_function_t<F>>::value>>> = true;

    template<class UDF>
    struct function;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
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
        quotedF._callable();
    };
#endif
}

namespace sqlite_orm::internal {
    template<class F, class SFINAE = void>
    struct callable_arguments_impl;

    template<class F>
    struct callable_arguments_impl<F, match_if<is_scalar_udf, F>> {
        using args_tuple = function_arguments<scalar_call_function_t<F>, std::tuple, std::decay_t>;
        using return_type = function_return_type_t<scalar_call_function_t<F>>;
    };

    template<class F>
    struct callable_arguments_impl<F, match_if<is_aggregate_udf, F>> {
        using args_tuple = function_arguments<aggregate_step_function_t<F>, std::tuple, std::decay_t>;
        using return_type = function_return_type_t<aggregate_fin_function_t<F>>;
    };

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<class F>
        requires (std::is_function_v<F>)
    struct callable_arguments_impl<F, void> {
        using args_tuple = function_arguments<F, std::tuple, std::decay_t>;
        using return_type = std::decay_t<function_return_type_t<F>>;
    };
#endif

    template<class F>
    struct callable_arguments : callable_arguments_impl<F> {};

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_function_sig Sig, class F>
    using overloaded_callop_t = decltype(static_cast<Sig F::*>(&F::operator()));

    template<orm_function_sig Sig, class F>
    using overloaded_static_callop_t =
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        decltype(static_cast<Sig*>(&F::operator()));
#else
        std::enable_if_t<polyfill::always_false_v<Sig>, void>;
#endif

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
        requires (requires { UDF::name(); })
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
                 std::enable_if_t<polyfill::negation<std::is_same<R, char>>::value, bool> = true>
        SQLITE_ORM_STATIC_CALLOP decltype(auto) operator()() SQLITE_ORM_OR_CONST_CALLOP {
            return UDF::name();
        }

        template<class R = decltype(UDF::name()), std::enable_if_t<std::is_same<R, char>::value, bool> = true>
        SQLITE_ORM_STATIC_CALLOP std::string operator()() SQLITE_ORM_OR_CONST_CALLOP {
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
    constexpr bool
        is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, function_call>::value>> = true;

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

    template<size_t I, class FnParam, class CallArg>
    SQLITE_ORM_CONSTEVAL bool expected_pointer_value() {
        static_assert(polyfill::always_false_v<FnParam, CallArg>, "Expected a pointer value for I-th argument");
        return false;
    }

    template<size_t I, class FnParam, class CallArg, class EnableIfTag = void>
    constexpr bool is_same_pvt_v = expected_pointer_value<I, FnParam, CallArg>();

    // Always allow binding nullptr to a pointer argument
    template<size_t I, class PointerArg>
    constexpr bool is_same_pvt_v<I, PointerArg, std::nullptr_t, polyfill::void_t<typename PointerArg::tag>> = true;
    // Always allow binding nullptr to a pointer argument
    template<size_t I, class P, class T, class D>
    constexpr bool is_same_pvt_v<I, pointer_arg<P, T>, pointer_binding<std::nullptr_t, T, D>, void> = true;

    template<size_t I, class PointerArgDataType, class BindingDataType>
    SQLITE_ORM_CONSTEVAL bool assert_same_pointer_data_type() {
        constexpr bool valid = std::is_convertible<BindingDataType*, PointerArgDataType*>::value;
        static_assert(valid, "Pointer data types of I-th argument do not match");
        return valid;
    }

#ifndef SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR
    template<size_t I, const char* PointerArg, const char* Binding>
    SQLITE_ORM_CONSTEVAL bool assert_same_pointer_tag() {
        constexpr bool valid = Binding == PointerArg;
        static_assert(valid, "Pointer types (tags) of I-th argument do not match");
        return valid;
    }
    template<size_t I, class PointerArg, class Binding>
    constexpr bool
        is_same_pvt_v<I, PointerArg, Binding, polyfill::void_t<typename PointerArg::tag, typename Binding::tag>> =
            assert_same_pointer_tag<I, PointerArg::tag::value, Binding::tag::value>() &&
            assert_same_pointer_data_type<I, typename PointerArg::qualified_type, typename Binding::qualified_type>();
#else
    template<size_t I, class PointerArg, class Binding>
    constexpr bool assert_same_pointer_tag() {
        constexpr bool valid = Binding::value == PointerArg::value;
        static_assert(valid, "Pointer types (tags) of I-th argument do not match");
        return valid;
    }

    template<size_t I, class PointerArg, class Binding>
    constexpr bool
        is_same_pvt_v<I, PointerArg, Binding, polyfill::void_t<typename PointerArg::tag, typename Binding::tag>> =
            assert_same_pointer_tag<I, typename PointerArg::tag, typename Binding::tag>();
#endif

    // not a pointer value, currently leave it unchecked
    template<size_t I, class FnParam, class CallArg>
    SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::false_type) {
        return true;
    }

    // check the type of pointer values
    template<size_t I, class FnParam, class CallArg>
    SQLITE_ORM_CONSTEVAL bool validate_pointer_value_type(std::true_type) {
        return is_same_pvt_v<I, FnParam, CallArg>;
    }

    template<class FnParams, class CallArgs>
    SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(polyfill::index_constant<size_t(-1)>) {
        return true;
    }
    template<class FnParams, class CallArgs, size_t I>
    SQLITE_ORM_CONSTEVAL bool validate_pointer_value_types(polyfill::index_constant<I>) {
        using func_param_type = std::tuple_element_t<I, FnParams>;
        using call_arg_type = unpacked_arg_t<std::tuple_element_t<I, CallArgs>>;

        constexpr bool valid = validate_pointer_value_type<I,
                                                           std::tuple_element_t<I, FnParams>,
                                                           unpacked_arg_t<std::tuple_element_t<I, CallArgs>>>(
            polyfill::bool_constant < (polyfill::is_specialization_of_v<func_param_type, pointer_arg>) ||
            (polyfill::is_specialization_of_v<call_arg_type, pointer_binding>) > {});

        return validate_pointer_value_types<FnParams, CallArgs>(polyfill::index_constant<I - 1>{}) && valid;
    }

    /*  
     *  Note: Currently the number of call arguments is checked and whether the types of pointer values match,
     *  but other call argument types are not checked against the parameter types of the function.
     */
    template<typename UDF, typename... CallArgs>
    SQLITE_ORM_CONSTEVAL void check_function_call() {
        using call_args_tuple = std::tuple<CallArgs...>;
        using function_params_tuple = typename callable_arguments<UDF>::args_tuple;
        constexpr size_t callArgsCount = std::tuple_size<call_args_tuple>::value;
        constexpr size_t functionParamsCount = std::tuple_size<function_params_tuple>::value;
        static_assert(std::is_same<function_params_tuple, std::tuple<arg_values>>::value ||
                          (callArgsCount == functionParamsCount &&
                           validate_pointer_value_types<function_params_tuple, call_args_tuple>(
                               polyfill::index_constant<std::min(functionParamsCount, callArgsCount) - 1>{})),
                      "Check the number and types of the function call arguments");
    }

    /*
     *  Generator of a user-defined function call in a sql query expression.
     *  
     *  Use the variable template `func<>` to instantiate.
     *  
     *  Calling the generator captures the parameters in a `function_call` expression.
     */
    template<class UDF>
    struct function {
        using udf_type = UDF;
        using callable_type = UDF;

        /*
         *  Generates the SQL function call expression.
         */
        template<typename... CallArgs>
        function_call<UDF, CallArgs...> operator()(CallArgs... callArgs) const {
            check_function_call<UDF, CallArgs...>();
            return {_udf_holder(), {std::forward<CallArgs>(callArgs)...}};
        }

        // returns a character range
        constexpr auto name() const {
            return _udf_holder()();
        }

        constexpr auto _udf_holder() const {
            return internal::udf_holder<UDF>{};
        }
    };

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /*
     *  Generator of a user-defined function call in a sql query expression.
     *  
     *  Use the string literal operator template `""_scalar.quote()` to quote
     *  a freestanding function, lambda or function object.
     *  
     *  Calling the generator captures the parameters in a `function_call` expression.
     *  
     *  Internal notes:
     *  1. The nested `udf_type` typename is deliberately chosen to be the function signature,
     *     and will be the abstracted version of the specified quoted function. 
     */
    template<class F, class Sig, size_t N>
    struct quoted_scalar_function {
        using udf_type = Sig;
        using callable_type = F;

        /*
         *  Generates the SQL function call expression.
         */
        template<typename... CallArgs>
        function_call<udf_type, CallArgs...> operator()(CallArgs... callArgs) const {
            check_function_call<udf_type, CallArgs...>();
            return {_udf_holder(), {std::forward<CallArgs>(callArgs)...}};
        }

        constexpr auto name() const {
            return _nme;
        }

        /*  
         *  Make the final callable that will be used to apply the user-defined function:
         *  - if `F` is a pointer to a freestanding function, return it as is
         *  - if `F` is a static call operator, return a pointer to it;
         *    the function signature is used to pick the function object's static call operator
         *  - if `F` is a stateless functor, bind a reference to the original function object its call operator;
         *    the function signature is used to pick the function object's call operator
         *  - if `F` is a stateful functor, bind a copy of the original function object to its call operator;
         *    the function signature is used to pick the function object's call operator
         */
        constexpr auto _callable() const {
            // function pointer
            if constexpr (std::is_pointer_v<F>) {
                return _udf;
            }
            // static call operator
            else if constexpr (polyfill::is_detected_v<internal::overloaded_static_callop_t, Sig, F>) {
                return static_cast<Sig*>(&F::operator());
            }
            // stateless functor
            else if constexpr (stateless<F>) {
#if __cpp_lib_bind_front >= 202306L  // NTTP callables
                return std::bind_front<static_cast<Sig F::*>(&F::operator())>(std::ref(_udf));
#else
                return std::bind_front(static_cast<Sig F::*>(&F::operator()), std::ref(_udf));
#endif
            }
            // stateful functor
            else {
                // non-const copy
#if __cpp_lib_bind_front >= 202306L  // NTTP callables
                return std::bind_front<static_cast<Sig F::*>(&F::operator())>(_udf);
#else
                return std::bind_front(static_cast<Sig F::*>(&F::operator()), _udf);
#endif
            }
        }

        constexpr auto _udf_holder() const {
            return internal::udf_holder<udf_type>{this->name()};
        }

        template<class... Args>
        consteval quoted_scalar_function(const char (&name)[N], Args&&... constructorArgs) :
            _udf(std::forward<Args>(constructorArgs)...) {
            std::copy_n(name, N, _nme);
        }

        F _udf;
        char _nme[N];
    };

    template<size_t N>
    struct quoted_function_builder : cstring_literal<N> {
        constexpr quoted_function_builder(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        /*
         *  From a freestanding function, possibly overloaded.
         */
        template<orm_function_sig F>
        [[nodiscard]] consteval auto quote(F* callable) const {
            return quoted_scalar_function<F*, F, N>{this->cstr, std::move(callable)};
        }

        /*
         *  From a classic function object instance.
         */
        template<orm_classic_function_object F>
            requires (stateless<F> || std::copy_constructible<F>)
        [[nodiscard]] consteval auto quote(F callable) const {
            using Sig = function_signature_type_t<decltype(&F::operator())>;
            return quoted_scalar_function<F, Sig, N>{this->cstr, std::move(callable)};
        }

        /*
         *  From a function object instance, picking the overloaded call operator.
         */
        template<orm_function_sig Sig, class F>
            requires (stateless<F> || std::copy_constructible<F>)
        [[nodiscard]] consteval auto quote(F callable) const {
            // detect whether overloaded call operator can be picked using `Sig`
            static_assert(polyfill::is_detected_v<overloaded_callop_t, Sig, F> ||
                              polyfill::is_detected_v<overloaded_static_callop_t, Sig, F>,
                          "No call operator with the specified signature available; have you overlooked the method "
                          "qualifiers?");
            return quoted_scalar_function<F, Sig, N>{this->cstr, std::move(callable)};
        }

        /*
         *  From a classic function object type.
         */
        template<orm_classic_function_object F, class... Args>
            requires (stateless<F> || std::copy_constructible<F>)
        [[nodiscard]] consteval auto quote(Args&&... constructorArgs) const {
            using Sig = function_signature_type_t<decltype(&F::operator())>;
            return quoted_scalar_function<F, Sig, N>{this->cstr, std::forward<Args>(constructorArgs)...};
        }

        /*
         *  From a function object type, picking the overloaded call operator.
         */
        template<orm_function_sig Sig, class F, class... Args>
            requires (stateless<F> || std::copy_constructible<F>)
        [[nodiscard]] consteval auto quote(Args&&... constructorArgs) const {
            // detect whether overloaded call operator can be picked using `Sig`
            static_assert(polyfill::is_detected_v<overloaded_callop_t, Sig, F> ||
                              polyfill::is_detected_v<overloaded_static_callop_t, Sig, F>,
                          "No call operator with the specified signature available; have you overlooked the method "
                          "qualifiers?");
            return quoted_scalar_function<F, Sig, N>{this->cstr, std::forward<Args>(constructorArgs)...};
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** @short Define a user-defined function.
     *  
     *  Note: Currently the number of call arguments is checked and whether the types of pointer values match,
     *  but other call argument types are not checked against the parameter types of the function.
     * 
     *  Example:
     *  struct IdFunc { int oeprator(int arg)() const { return arg; } };
     *  // inline:
     *  select(func<IdFunc>(42));
     *  // As this is a variable template, you can frame the user-defined function and define a variable for syntactic sugar and legibility:
     *  inline constexpr orm_scalar_function auto idfunc = func<IdFunc>;
     *  select(idfunc(42));
     *  
     */
    template<class UDF>
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        requires (orm_scalar_udf<UDF> || orm_aggregate_udf<UDF>)
#endif
    inline constexpr internal::function<UDF> func{};

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline namespace literals {
        /*  @short Create a scalar function from a freestanding function, lambda or function object,
         *  and call such a user-defined function.
         *  
         *  If you need to pick a function or method from an overload set, or pick a template function you can
         *  specify an explicit function signature in the call to `quote()`.
         *  
         *  Examples:
         *  // freestanding function from a library
         *  constexpr orm_quoted_scalar_function auto clamp_int_f = "clamp_int"_scalar.quote(std::clamp<int>);
         *  // stateless lambda
         *  constexpr orm_quoted_scalar_function auto is_fatal_error_f = "IS_FATAL_ERROR"_scalar.quote([](unsigned long errcode) static {
         *      return errcode != 0;
         *  });
         *  // function object instance
         *  constexpr orm_quoted_scalar_function auto equal_to_int_f = "equal_to"_scalar.quote(std::equal_to<int>{});
         *  // function object
         *  constexpr orm_quoted_scalar_function auto equal_to_int_2_f = "equal_to"_scalar.quote<std::equal_to<int>>();
         *  // pick function object's template call operator
         *  constexpr orm_quoted_scalar_function auto equal_to_int_3_f = "equal_to"_scalar.quote<bool(const int&, const int&) const>(std::equal_to<void>{});
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
        [[nodiscard]] consteval auto operator""_scalar() {
            return builder;
        }
    }
#endif
}
