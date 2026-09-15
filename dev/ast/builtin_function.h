#pragma once

/** @file The nodes of a call of a built-in SQL function: scalar and aggregate function calls,
 *        an aggregate call with a FILTER clause, and COUNT(*).
 *        In C++20 builds, also the definition of a built-in function by its name and its overload set,
 *        which generates the call nodes. The functions themselves are in `core_functions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same, std::is_void, std::type_identity, std::conditional, std::remove_cvref
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#include <string_view>  //  std::string_view
#include <utility>  //  std::move, std::forward
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>  //  std::convertible_to
#include <algorithm>  //  std::copy_n
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/is_base_template_of.h"
#include "../functional/type_traits.h"  //  satisfies, orm_function_sig
#include "../functional/cstring_literal.h"
#include "../functional/function_traits.h"  //  function_arguments, function_return_type_t
#include "../tags.h"  //  arithmetic_t
#include "../vocabulary/node_traits.h"  //  is_where, expression_type_t
#include "../vocabulary/node_algorithms.h"  //  argument, common_argument_type
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits
#include "window.h"  //  over_t, validate_over_arguments

namespace sqlite_orm::internal {
    /*
     *  Represents an aggregate function call with a FILTER clause,
     *  which may be turned into a window function with an OVER clause.
     */
    template<class F, class W>
    struct filtered_aggregate_function {
        using function_type = F;
        using where_expression = W;

        function_type function;
        where_expression where;

        template<class... OverArgs>
        over_t<filtered_aggregate_function, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class T>
    constexpr bool is_filtered_aggregate_function_v = polyfill::is_specialization_of_v<T, filtered_aggregate_function>;

    struct count_string {
        std::string_view serialize() const {
            return "COUNT";
        }
    };

    /**
     *  T is use to specify type explicitly for queries like
     *  SELECT COUNT(*) FROM table_name;
     *  T can be omitted with void.
     */
    template<class T>
    struct count_asterisk_t : count_string {
        using type = T;

        template<class Wh, satisfies<is_where, Wh> = true>
        filtered_aggregate_function<count_asterisk_t<T>, expression_type_t<Wh>> filter(Wh wh) {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        over_t<count_asterisk_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    /**
     *  The same thing as count<T>() but without T arg.
     *  Is used in cases like this:
     *    SELECT cust_code, cust_name, cust_city, grade
     *    FROM customer
     *    WHERE grade=2 AND EXISTS
     *        (SELECT COUNT(*)
     *        FROM customer
     *        WHERE grade=2
     *        GROUP BY grade
     *        HAVING COUNT(*)>2);
     *  `c++`
     *  auto rows =
     *      storage.select(columns(&Customer::code, &Customer::name, &Customer::city, &Customer::grade),
     *          where(is_equal(&Customer::grade, 2)
     *              and exists(select(count<Customer>(),
     *                  where(is_equal(&Customer::grade, 2)),
     *          group_by(&Customer::grade),
     *          having(greater_than(count(), 2))))));
     */
    struct count_asterisk_without_type : count_string {
        using type = void;
    };

    template<class T>
    constexpr bool is_count_asterisk_v =
        polyfill::is_specialization_of_v<T, count_asterisk_t> || std::is_same_v<T, count_asterisk_without_type>;

    template<class T>
    constexpr bool is_operator_argument_v<T, std::enable_if_t<is_count_asterisk_v<T>>> = true;
}

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The legacy built-in function nodes, superseded by the definition mechanism below in C++20 builds.
     *  Their return type may use the placeholders of `vocabulary/algorithms/argument_placeholders.h` all the same.
     */

    /**
     *  Base class for operator overloading
     *  R - return type
     *  S - class with operator std::string
     *  Args - function arguments types
     */
    template<class R, class S, class... Args>
    struct builtin_function_t : S, arithmetic_t {
        using return_type = R;
        using string_type = S;
        using args_tuple = std::tuple<Args...>;

        static constexpr size_t args_size = std::tuple_size<args_tuple>::value;

        args_tuple args;

        constexpr builtin_function_t(args_tuple&& args_) : args(std::move(args_)) {}
    };

    template<class T>
    constexpr bool is_builtin_function_v = is_base_template_of<builtin_function_t, T>::value;

    template<class R, class S, class... Args>
    struct builtin_aggregate_function_t : builtin_function_t<R, S, Args...> {
        using super = builtin_function_t<R, S, Args...>;

        using super::super;

        template<class Wh, satisfies<is_where, Wh> = true>
        filtered_aggregate_function<builtin_aggregate_function_t, expression_type_t<Wh>> filter(Wh wh) {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        over_t<builtin_aggregate_function_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };
}
#else
/*
 *  Signature vocabulary of built-in functions (see also the return type placeholders
 *  `argument<I>` and `common_argument_type<I...>` in `vocabulary/algorithms/argument_placeholders.h`).
 *
 *  Parameter types are nominal: no callable receives them and call arguments are not checked against them;
 *  they state the SQL contract and fix the arity. The return type is not nominal - it is what a select yields.
 */
namespace sqlite_orm::internal {
    template<class T>
    constexpr bool is_builtin_function_v = false;

    /*
     *  Marker for the last parameter of a built-in function's signature: "zero or more further `T`".
     *
     *  The parameters preceding the marker are required, so the minimum arity follows from its position.
     *
     *  Example:
     *  "COALESCE"_builtin.scalar<argument<0>(anything, anything, variadic<anything>)>()
     */
    template<class T>
    struct variadic {};

    /*
     *  Parameter type of a built-in function that is polymorphic in SQL, e.g. compared by collation.
     */
    struct anything {};

    /*
     *  A built-in function's signature, tagged with its kind.
     */
    template<orm_function_sig Sig>
    struct scalar_sig {
        using signature_type = Sig;
    };

    template<orm_function_sig Sig>
    struct aggregate_sig {
        using signature_type = Sig;
    };

    template<class T>
    constexpr bool is_kinded_signature_v = false;
    template<class Sig>
    constexpr bool is_kinded_signature_v<scalar_sig<Sig>> = true;
    template<class Sig>
    constexpr bool is_kinded_signature_v<aggregate_sig<Sig>> = true;

    /*
     *  Whether a built-in function's signature accepts a call with `Argc` arguments.
     */
    template<orm_function_sig Sig, size_t Argc>
    consteval bool builtin_signature_accepts() {
        using params_tuple = function_arguments<Sig, std::tuple>;
        constexpr size_t paramsCount = std::tuple_size_v<params_tuple>;
        if constexpr (paramsCount == 0) {
            return Argc == 0;
        } else if constexpr (polyfill::is_specialization_of_v<std::tuple_element_t<paramsCount - 1, params_tuple>,
                                                              variadic>) {
            return Argc >= paramsCount - 1;
        } else {
            return Argc == paramsCount;
        }
    }

    /*
     *  The first kinded signature of a built-in function's overload set accepting a call with `Argc` arguments.
     *  Has no nested `type` if none does.
     */
    template<size_t Argc, class... KindedSigs>
    struct matched_builtin_signature {};

    template<size_t Argc, class KindedSig, class... KindedSigs>
    struct matched_builtin_signature<Argc, KindedSig, KindedSigs...>
        : std::conditional_t<builtin_signature_accepts<typename KindedSig::signature_type, Argc>(),
                             std::type_identity<KindedSig>,
                             matched_builtin_signature<Argc, KindedSigs...>> {};

    template<size_t Argc, class... KindedSigs>
    using matched_builtin_signature_t = typename matched_builtin_signature<Argc, KindedSigs...>::type;

    /*
     *  Whether a built-in function's overload set has a signature accepting a call with `Argc` arguments.
     */
    template<size_t Argc, class... KindedSigs>
    concept has_matching_builtin_signature = requires { typename matched_builtin_signature_t<Argc, KindedSigs...>; };

    /*
     *  A kinded signature with its return type replaced by `R`, or as is if `R` is `void`.
     */
    template<class R, class KindedSig>
    struct with_return_type;

    template<class R, class R0, class... Params>
    struct with_return_type<R, scalar_sig<R0(Params...)>>
        : std::type_identity<scalar_sig<std::conditional_t<std::is_void_v<R>, R0, R>(Params...)>> {};

    template<class R, class R0, class... Params>
    struct with_return_type<R, aggregate_sig<R0(Params...)>>
        : std::type_identity<aggregate_sig<std::conditional_t<std::is_void_v<R>, R0, R>(Params...)>> {};

    template<class R, class KindedSig>
    using with_return_type_t = typename with_return_type<R, KindedSig>::type;

    /*
     *  Represents a call of a built-in scalar function.
     *
     *  `F` is the definition type of the built-in function, `Sig` the matched overload.
     */
    template<class F, class Sig, class... CallArgs>
    struct builtin_function_call : arithmetic_t {
        using function_type = F;
        using signature_type = Sig;
        using return_type = function_return_type_t<Sig>;
        using args_tuple = std::tuple<CallArgs...>;

        SQLITE_ORM_NOUNIQUEADDRESS function_type function;
        args_tuple args;

        constexpr builtin_function_call(function_type function_, args_tuple args_) :
            function{std::move(function_)}, args{std::move(args_)} {}

        constexpr std::string_view serialize() const {
            return this->function.name();
        }
    };

    /*
     *  Represents a call of a built-in aggregate function, which may take a FILTER clause
     *  or be turned into a window function with an OVER clause.
     */
    template<class F, class Sig, class... CallArgs>
    struct builtin_aggregate_function_call : builtin_function_call<F, Sig, CallArgs...> {
        using super = builtin_function_call<F, Sig, CallArgs...>;

        using super::super;

        template<class Wh>
            requires (is_where_v<Wh>)
        constexpr filtered_aggregate_function<builtin_aggregate_function_call, expression_type_t<Wh>>
        filter(Wh wh) const {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        constexpr over_t<builtin_aggregate_function_call, OverArgs...> over(OverArgs... overArgs) const {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_builtin_function_v<builtin_function_call<F, Sig, CallArgs...>> = true;
    template<class F, class Sig, class... CallArgs>
    constexpr bool is_builtin_function_v<builtin_aggregate_function_call<F, Sig, CallArgs...>> = true;

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_operator_argument_v<builtin_function_call<F, Sig, CallArgs...>, void> = true;
    template<class F, class Sig, class... CallArgs>
    constexpr bool is_operator_argument_v<builtin_aggregate_function_call<F, Sig, CallArgs...>, void> = true;

    /*
     *  The call node for a matched kinded signature.
     */
    template<class KindedSig, class F, class... CallArgs>
    struct builtin_function_call_for;

    template<class Sig, class F, class... CallArgs>
    struct builtin_function_call_for<scalar_sig<Sig>, F, CallArgs...>
        : std::type_identity<builtin_function_call<F, Sig, CallArgs...>> {};

    template<class Sig, class F, class... CallArgs>
    struct builtin_function_call_for<aggregate_sig<Sig>, F, CallArgs...>
        : std::type_identity<builtin_aggregate_function_call<F, Sig, CallArgs...>> {};

    template<class KindedSig, class F, class... CallArgs>
    using builtin_function_call_for_t = typename builtin_function_call_for<KindedSig, F, CallArgs...>::type;

    /*
     *  Generator of a built-in function call in a sql query expression.
     *
     *  Use the string literal operator template `""_builtin.function<KindedSig...>()`
     *  - or the single-kind shorthands `.scalar<Sig...>()` and `.aggregate<Sig...>()` -
     *  to define a built-in function by its name and its overload set.
     *
     *  Calling the generator picks the overload by the number of call arguments
     *  and captures the arguments in a call expression of the overload's kind.
     */
    template<size_t N, class... KindedSigs>
        requires (is_kinded_signature_v<KindedSigs> && ...)
    struct builtin_function {
        using signature_tuple = std::tuple<KindedSigs...>;

        /*
         *  Generates the SQL function call expression.
         *
         *  An explicitly specified `R` replaces the matched overload's return type:
         *  `f.template operator()<std::optional<double>>(x)`. This is what the function template facades
         *  of the built-in functions taking the return type as a template argument (`acos<std::optional<double>>(x)`)
         *  are implemented with.
         */
        template<class R = void, class... CallArgs>
            requires (has_matching_builtin_signature<sizeof...(CallArgs), KindedSigs...>)
        constexpr builtin_function_call_for_t<
            with_return_type_t<R, matched_builtin_signature_t<sizeof...(CallArgs), KindedSigs...>>,
            builtin_function,
            CallArgs...>
        operator()(CallArgs... callArgs) const {
            return {*this, {std::forward<CallArgs>(callArgs)...}};
        }

        constexpr std::string_view name() const {
            return {_nme, N - 1};
        }

        consteval builtin_function(const char (&name)[N]) {
            std::copy_n(name, N, _nme);
        }

        char _nme[N];
    };

    template<size_t N>
    struct builtin_function_builder : cstring_literal<N> {
        constexpr builtin_function_builder(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        /*
         *  A function with the given overload set of kinded signatures, in any order.
         */
        template<class... KindedSigs>
            requires (sizeof...(KindedSigs) > 0) && (is_kinded_signature_v<KindedSigs> && ...)
        [[nodiscard]] consteval auto function() const {
            return builtin_function<N, KindedSigs...>{this->cstr};
        }

        /*
         *  A scalar function with the given overload set.
         */
        template<orm_function_sig... Sigs>
            requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto scalar() const {
            return builtin_function<N, scalar_sig<Sigs>...>{this->cstr};
        }

        /*
         *  An aggregate function with the given overload set.
         */
        template<orm_function_sig... Sigs>
            requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto aggregate() const {
            return builtin_function<N, aggregate_sig<Sigs>...>{this->cstr};
        }
    };

    /*  @short Define a built-in function by its name and its overload set.
     *
     *  Deliberately internal: sqlite_orm defines all built-in functions itself.
     *
     *  Examples:
     *  inline constexpr auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
     *  inline constexpr auto substr = "SUBSTR"_builtin.scalar<std::string(std::string_view, int), std::string(std::string_view, int, int)>();
     *  inline constexpr auto max = "MAX"_builtin.function<aggregate_sig<std::unique_ptr<argument<0>>(anything)>,
     *                                                     scalar_sig<std::unique_ptr<argument<0>>(anything, anything, variadic<anything>)>>();
     */
    template<builtin_function_builder builder>
    [[nodiscard]] consteval auto operator""_builtin() {
        return builder;
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** @short Specifies that a type is a built-in function definition.
     */
    template<class F>
    concept orm_builtin_function = requires(const F& f) {
        { f.name() } -> std::convertible_to<std::string_view>;
        typename std::remove_cvref_t<F>::signature_tuple;
    };
}
#endif
