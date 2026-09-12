#pragma once

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_void, std::type_identity, std::conditional, std::remove_cvref
#include <concepts>  //  std::convertible_to
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#include <string_view>  //  std::string_view
#include <algorithm>  //  std::copy_n
#include <utility>  //  std::move, std::forward
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/cstring_literal.h"
#include "../functional/function_traits.h"
#include "../function.h"  // orm_function_sig
#include "../tags.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** @short Marker for the last parameter of a built-in function's signature: "zero or more further `T`".
     *
     *  The parameters preceding the marker are required, so the minimum arity follows from its position.
     *
     *  Example:
     *  "MAX"_builtin.scalar<double(double, double, variadic<double>)>()
     */
    template<class T>
    struct variadic {};
}

namespace sqlite_orm::internal {
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
     *  The first signature of a built-in function's overload set accepting a call with `Argc` arguments,
     *  or `void`.
     */
    template<size_t Argc, class... Sigs>
    struct matched_built_in_signature : std::type_identity<void> {};

    template<size_t Argc, class Sig, class... Sigs>
    struct matched_built_in_signature<Argc, Sig, Sigs...>
        : std::conditional_t<built_in_signature_accepts<Sig, Argc>(),
                             std::type_identity<Sig>,
                             matched_built_in_signature<Argc, Sigs...>> {};

    template<size_t Argc, class... Sigs>
    using matched_built_in_signature_t = typename matched_built_in_signature<Argc, Sigs...>::type;

    /*
     *  Represents a call of a built-in function.
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

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_built_in_function_v<built_in_function_call<F, Sig, CallArgs...>> = true;

    template<class F, class Sig, class... CallArgs>
    constexpr bool is_operator_argument_v<built_in_function_call<F, Sig, CallArgs...>, void> = true;

    /*
     *  Generator of a built-in scalar function call in a sql query expression.
     *
     *  Use the string literal operator template `""_builtin.scalar<Sig...>()` to define
     *  a built-in scalar function by its name and its overload set.
     *
     *  Calling the generator picks the overload by the number of call arguments
     *  and captures the arguments in a `built_in_function_call` expression.
     */
    template<size_t N, orm_function_sig... Sigs>
    struct built_in_scalar_function {
        using signature_tuple = std::tuple<Sigs...>;

        /*
         *  Generates the SQL function call expression.
         */
        template<class... CallArgs>
            requires (!std::is_void_v<matched_built_in_signature_t<sizeof...(CallArgs), Sigs...>>)
        constexpr built_in_function_call<built_in_scalar_function,
                                         matched_built_in_signature_t<sizeof...(CallArgs), Sigs...>,
                                         CallArgs...>
        operator()(CallArgs... callArgs) const {
            return {*this, {std::forward<CallArgs>(callArgs)...}};
        }

        constexpr std::string_view name() const {
            return {_nme, N - 1};
        }

        consteval built_in_scalar_function(const char (&name)[N]) {
            std::copy_n(name, N, _nme);
        }

        char _nme[N];
    };

    template<size_t N>
    struct built_in_function_builder : cstring_literal<N> {
        constexpr built_in_function_builder(const char (&cstr)[N]) : cstring_literal<N>{cstr} {}

        /*
         *  A scalar function with the given overload set.
         */
        template<orm_function_sig... Sigs>
            requires (sizeof...(Sigs) > 0)
        [[nodiscard]] consteval auto scalar() const {
            return built_in_scalar_function<N, Sigs...>{this->cstr};
        }
    };

    /*  @short Define a built-in function by its name and its overload set.
     *
     *  Deliberately internal: sqlite_orm defines all built-in functions itself.
     *
     *  Examples:
     *  inline constexpr auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
     *  inline constexpr auto substr = "SUBSTR"_builtin.scalar<std::string(std::string_view, int), std::string(std::string_view, int, int)>();
     *  inline constexpr auto max = "MAX"_builtin.scalar<double(double, double, variadic<double>)>();
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
