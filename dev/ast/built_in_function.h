#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <type_traits>  //  std::is_void, std::type_identity, std::conditional, std::remove_cvref
#include <concepts>  //  std::convertible_to
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#include <string_view>  //  std::string_view
#include <algorithm>  //  std::copy_n
#include <utility>  //  std::move, std::forward
#endif
#endif

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cstring_literal.h"
#include "../functional/function_traits.h"
#include "../function.h"  // orm_function_sig
#include "../tags.h"
#include "../vocabulary/node_traits.h"  // is_where_v, expression_type_t
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits
#include "window.h"  // over_t, validate_over_arguments
#endif

/*
 *  Signature vocabulary of built-in functions.
 *
 *  Parameter types are nominal: no callable receives them and call arguments are not checked against them;
 *  they state the SQL contract and fix the arity. The return type is not nominal - it is what a select yields.
 *
 *  The markers are declared unconditionally so that the vocabulary algorithms can name them;
 *  the definition mechanism itself requires C++20.
 */
namespace sqlite_orm::internal {
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
     *  Placeholder in the return type of a built-in function's signature,
     *  standing for the result type of the I-th call argument.
     *
     *  It may appear anywhere inside the return type, e.g. `std::unique_ptr<argument<0>>`.
     */
    template<size_t I>
    struct argument {};
}

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    template<class F, class W>
    struct filtered_aggregate_function;

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
    consteval bool built_in_signature_accepts() {
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
     *  The first kinded signature of a built-in function's overload set accepting a call with `Argc` arguments,
     *  or `void`.
     */
    template<size_t Argc, class... KindedSigs>
    struct matched_built_in_signature : std::type_identity<void> {};

    template<size_t Argc, class KindedSig, class... KindedSigs>
    struct matched_built_in_signature<Argc, KindedSig, KindedSigs...>
        : std::conditional_t<built_in_signature_accepts<typename KindedSig::signature_type, Argc>(),
                             std::type_identity<KindedSig>,
                             matched_built_in_signature<Argc, KindedSigs...>> {};

    template<size_t Argc, class... KindedSigs>
    using matched_built_in_signature_t = typename matched_built_in_signature<Argc, KindedSigs...>::type;

    /*
     *  Represents a call of a built-in scalar function.
     *
     *  `F` is the definition type of the built-in function, `Sig` the matched overload.
     */
    template<class F, class Sig, class... CallArgs>
    struct built_in_function_call : arithmetic_t {
        using function_type = F;
        using signature_type = Sig;
        using return_type = function_return_type_t<Sig>;
        using args_type = std::tuple<CallArgs...>;

        SQLITE_ORM_NOUNIQUEADDRESS function_type function;
        args_type args;

        constexpr built_in_function_call(function_type function_, args_type args_) :
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
    struct built_in_aggregate_function_call : built_in_function_call<F, Sig, CallArgs...> {
        using super = built_in_function_call<F, Sig, CallArgs...>;

        using super::super;

        template<class Wh>
            requires (is_where_v<Wh>)
        constexpr filtered_aggregate_function<built_in_aggregate_function_call, expression_type_t<Wh>>
        filter(Wh wh) const {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        constexpr over_t<built_in_aggregate_function_call, OverArgs...> over(OverArgs... overArgs) const {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_built_in_function_v<built_in_function_call<F, Sig, CallArgs...>> = true;
    template<class F, class Sig, class... CallArgs>
    constexpr bool is_built_in_function_v<built_in_aggregate_function_call<F, Sig, CallArgs...>> = true;

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_operator_argument_v<built_in_function_call<F, Sig, CallArgs...>, void> = true;
    template<class F, class Sig, class... CallArgs>
    constexpr bool is_operator_argument_v<built_in_aggregate_function_call<F, Sig, CallArgs...>, void> = true;

    /*
     *  The call node for a matched kinded signature.
     */
    template<class KindedSig, class F, class... CallArgs>
    struct built_in_function_call_for;

    template<class Sig, class F, class... CallArgs>
    struct built_in_function_call_for<scalar_sig<Sig>, F, CallArgs...>
        : std::type_identity<built_in_function_call<F, Sig, CallArgs...>> {};

    template<class Sig, class F, class... CallArgs>
    struct built_in_function_call_for<aggregate_sig<Sig>, F, CallArgs...>
        : std::type_identity<built_in_aggregate_function_call<F, Sig, CallArgs...>> {};

    template<class KindedSig, class F, class... CallArgs>
    using built_in_function_call_for_t = typename built_in_function_call_for<KindedSig, F, CallArgs...>::type;

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
    struct built_in_function {
        using signature_tuple = std::tuple<KindedSigs...>;

        /*
         *  Generates the SQL function call expression.
         */
        template<class... CallArgs>
            requires (!std::is_void_v<matched_built_in_signature_t<sizeof...(CallArgs), KindedSigs...>>)
        constexpr built_in_function_call_for_t<matched_built_in_signature_t<sizeof...(CallArgs), KindedSigs...>,
                                               built_in_function,
                                               CallArgs...>
        operator()(CallArgs... callArgs) const {
            return {*this, {std::forward<CallArgs>(callArgs)...}};
        }

        constexpr std::string_view name() const {
            return {_nme, N - 1};
        }

        consteval built_in_function(const char (&name)[N]) {
            std::copy_n(name, N, _nme);
        }

        char _nme[N];
    };

    template<size_t N>
    struct built_in_function_builder : cstring_literal<N> {
        constexpr built_in_function_builder(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        /*
         *  A function with the given overload set of kinded signatures, in any order.
         */
        template<class... KindedSigs>
            requires (sizeof...(KindedSigs) > 0) && (is_kinded_signature_v<KindedSigs> && ...)
        [[nodiscard]] consteval auto function() const {
            return built_in_function<N, KindedSigs...>{this->cstr};
        }

        /*
         *  A scalar function with the given overload set.
         */
        template<orm_function_sig... Sigs>
            requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto scalar() const {
            return built_in_function<N, scalar_sig<Sigs>...>{this->cstr};
        }

        /*
         *  An aggregate function with the given overload set.
         */
        template<orm_function_sig... Sigs>
            requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto aggregate() const {
            return built_in_function<N, aggregate_sig<Sigs>...>{this->cstr};
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
    template<built_in_function_builder builder>
    [[nodiscard]] consteval auto operator""_builtin() {
        return builder;
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** @short Specifies that a type is a built-in function definition.
     */
    template<class F>
    concept orm_built_in_function = requires(const F& f) {
        { f.name() } -> std::convertible_to<std::string_view>;
        typename std::remove_cvref_t<F>::signature_tuple;
    };
}
#endif
