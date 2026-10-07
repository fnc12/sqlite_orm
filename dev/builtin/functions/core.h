#pragma once

/** @file The built-in core functions https://www.sqlite.org/lang_corefunc.html, the scalar functions not
 *        grouped elsewhere - except MAX(X,Y,...) and MIN(X,Y,...), which share their definitions with the
 *        aggregate functions of the same name in `aggregate.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <stdexcept>  //  std::domain_error
#include <tuple>  //  std::tuple, std::make_tuple
#include <type_traits>  //  std::enable_if, std::conditional, std::is_void, std::is_member_pointer, std::is_constant_evaluated
#include <utility>  //  std::move, std::forward
#include <memory>  //  std::unique_ptr
#include <vector>  //  std::vector
#include <optional>  //  std::optional
#include <string_view>  //  std::string_view
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../tuple_helper/tuple_traits.h"  //  count_tuple
#include "../../ast/literal.h"  //  literal_holder
#include "../../vocabulary/node_traits.h"  //  is_into, is_column_pointer
#include "../../vocabulary/node_algorithms.h"  //  argument, common_argument_type
#include "../../ast/builtin_function.h"
#include "../../sqlite3/sqlite3_types.h"  //  int64

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in core function nodes.
     */
    struct typeof_string {
        std::string_view serialize() const {
            return "TYPEOF";
        }
    };

    struct unicode_string {
        std::string_view serialize() const {
            return "UNICODE";
        }
    };

    struct length_string {
        std::string_view serialize() const {
            return "LENGTH";
        }
    };

    struct abs_string {
        std::string_view serialize() const {
            return "ABS";
        }
    };

    struct lower_string {
        std::string_view serialize() const {
            return "LOWER";
        }
    };

    struct upper_string {
        std::string_view serialize() const {
            return "UPPER";
        }
    };

    struct last_insert_rowid_string {
        std::string_view serialize() const {
            return "LAST_INSERT_ROWID";
        }
    };

    struct total_changes_string {
        std::string_view serialize() const {
            return "TOTAL_CHANGES";
        }
    };

    struct changes_string {
        std::string_view serialize() const {
            return "CHANGES";
        }
    };

    struct trim_string {
        std::string_view serialize() const {
            return "TRIM";
        }
    };

    struct ltrim_string {
        std::string_view serialize() const {
            return "LTRIM";
        }
    };

    struct rtrim_string {
        std::string_view serialize() const {
            return "RTRIM";
        }
    };

    struct hex_string {
        std::string_view serialize() const {
            return "HEX";
        }
    };

    struct quote_string {
        std::string_view serialize() const {
            return "QUOTE";
        }
    };

    struct randomblob_string {
        std::string_view serialize() const {
            return "RANDOMBLOB";
        }
    };

    struct instr_string {
        std::string_view serialize() const {
            return "INSTR";
        }
    };

    struct replace_string {
        std::string_view serialize() const {
            return "REPLACE";
        }
    };

    struct round_string {
        std::string_view serialize() const {
            return "ROUND";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3007016
    struct char_string {
        std::string_view serialize() const {
            return "CHAR";
        }
    };

    struct random_string {
        std::string_view serialize() const {
            return "RANDOM";
        }
    };
#endif
    struct sqlite_version_string {
        std::string_view serialize() const {
            return "SQLITE_VERSION";
        }
    };

    struct sqlite_source_id_string {
        std::string_view serialize() const {
            return "SQLITE_SOURCE_ID";
        }
    };

#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    struct sqlite_compileoption_used_string {
        std::string_view serialize() const {
            return "SQLITE_COMPILEOPTION_USED";
        }
    };

    struct sqlite_compileoption_get_string {
        std::string_view serialize() const {
            return "SQLITE_COMPILEOPTION_GET";
        }
    };
#endif
#ifdef SQLITE_ENABLE_OFFSET_SQL_FUNC
    struct sqlite_offset_string {
        std::string_view serialize() const {
            return "SQLITE_OFFSET";
        }
    };
#endif
    struct coalesce_string {
        std::string_view serialize() const {
            return "COALESCE";
        }
    };

    struct ifnull_string {
        std::string_view serialize() const {
            return "IFNULL";
        }
    };

    struct nullif_string {
        std::string_view serialize() const {
            return "NULLIF";
        }
    };

    struct zeroblob_string {
        std::string_view serialize() const {
            return "ZEROBLOB";
        }
    };

    struct substr_string {
        std::string_view serialize() const {
            return "SUBSTR";
        }
    };

#ifdef SQLITE_SOUNDEX
    struct soundex_string {
        std::string_view serialize() const {
            return "SOUNDEX";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3008001
    struct likelihood_string {
        std::string_view serialize() const {
            return "LIKELIHOOD";
        }
    };

    struct unlikely_string {
        std::string_view serialize() const {
            return "UNLIKELY";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    struct printf_string {
        std::string_view serialize() const {
            return "PRINTF";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3008006
    struct likely_string {
        std::string_view serialize() const {
            return "LIKELY";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3032000
    struct iif_string {
        std::string_view serialize() const {
            return "IIF";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3034000
    struct substring_string {
        std::string_view serialize() const {
            return "SUBSTRING";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3035000
    struct sign_string {
        std::string_view serialize() const {
            return "SIGN";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3038000
    struct format_string {
        std::string_view serialize() const {
            return "FORMAT";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3041000
    struct unhex_string {
        std::string_view serialize() const {
            return "UNHEX";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    struct octet_length_string {
        std::string_view serialize() const {
            return "OCTET_LENGTH";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3044000
    struct concat_string {
        std::string_view serialize() const {
            return "CONCAT";
        }
    };

    struct concat_ws_string {
        std::string_view serialize() const {
            return "CONCAT_WS";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3048000
    struct if_string {
        std::string_view serialize() const {
            return "IF";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3050000
    struct unistr_string {
        std::string_view serialize() const {
            return "UNISTR";
        }
    };

    struct unistr_quote_string {
        std::string_view serialize() const {
            return "UNISTR_QUOTE";
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  TYPEOF(x) function https://sqlite.org/lang_corefunc.html#typeof
     */
    template<class T>
    constexpr internal::builtin_function_t<std::string, internal::typeof_string, T> typeof_(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  UNICODE(x) function https://sqlite.org/lang_corefunc.html#unicode
     */
    template<class T>
    constexpr internal::builtin_function_t<int, internal::unicode_string, T> unicode(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
     */
    template<class T>
    constexpr internal::builtin_function_t<int, internal::length_string, T> length(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
     */
    template<class T>
    constexpr internal::builtin_function_t<std::unique_ptr<double>, internal::abs_string, T> abs(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
     */
    template<class T>
    constexpr internal::builtin_function_t<std::string, internal::lower_string, T> lower(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
     */
    template<class T>
    constexpr internal::builtin_function_t<std::string, internal::upper_string, T> upper(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  LAST_INSERT_ROWID() function https://www.sqlite.org/lang_corefunc.html#last_insert_rowid
     */
    constexpr internal::builtin_function_t<int64, internal::last_insert_rowid_string> last_insert_rowid() {
        return {{}};
    }

    /**
     *  TOTAL_CHANGES() function https://sqlite.org/lang_corefunc.html#total_changes
     */
    constexpr internal::builtin_function_t<int, internal::total_changes_string> total_changes() {
        return {{}};
    }

    /**
     *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
     */
    constexpr internal::builtin_function_t<int, internal::changes_string> changes() {
        return {{}};
    }

    /**
     *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class T>
    constexpr internal::builtin_function_t<std::string, internal::trim_string, T> trim(T t) {
        return {std::tuple<T>{std::forward<T>(t)}};
    }

    /**
     *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::trim_string, X, Y> trim(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::ltrim_string, X> ltrim(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::ltrim_string, X, Y> ltrim(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::rtrim_string, X> rtrim(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::rtrim_string, X, Y> rtrim(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  HEX(X) function https://sqlite.org/lang_corefunc.html#hex
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::hex_string, X> hex(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  QUOTE(X) function https://sqlite.org/lang_corefunc.html#quote
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::quote_string, X> quote(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  RANDOMBLOB(X) function https://sqlite.org/lang_corefunc.html#randomblob
     */
    template<class X>
    constexpr internal::builtin_function_t<std::vector<char>, internal::randomblob_string, X> randomblob(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  INSTR(X) function https://sqlite.org/lang_corefunc.html#instr
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<int, internal::instr_string, X, Y> instr(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  REPLACE(X) function https://sqlite.org/lang_corefunc.html#replace
     */
    template<class X,
             class Y,
             class Z,
             std::enable_if_t<internal::count_tuple<std::tuple<X, Y, Z>, internal::is_into>::value == 0, bool> = true>
    constexpr internal::builtin_function_t<std::string, internal::replace_string, X, Y, Z> replace(X x, Y y, Z z) {
        return {std::tuple<X, Y, Z>{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)}};
    }

    /**
     *  ROUND(X) function https://sqlite.org/lang_corefunc.html#round
     */
    template<class X>
    constexpr internal::builtin_function_t<double, internal::round_string, X> round(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ROUND(X, Y) function https://sqlite.org/lang_corefunc.html#round
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<double, internal::round_string, X, Y> round(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

#if SQLITE_VERSION_NUMBER >= 3007016
    /**
     *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::char_string, Args...> char_(Args... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  RANDOM() function https://www.sqlite.org/lang_corefunc.html#random
     */
    constexpr internal::builtin_function_t<int, internal::random_string> random() {
        return {{}};
    }
#endif
    /**
     *  SQLITE_VERSION() function https://www.sqlite.org/lang_corefunc.html#sqlite_version
     */
    constexpr internal::builtin_function_t<std::string, internal::sqlite_version_string> sqlite_version() {
        return {{}};
    }

    /**
     *  SQLITE_SOURCE_ID() function https://www.sqlite.org/lang_corefunc.html#sqlite_source_id
     */
    constexpr internal::builtin_function_t<std::string, internal::sqlite_source_id_string> sqlite_source_id() {
        return {{}};
    }

#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    /**
     *  SQLITE_COMPILEOPTION_USED(X) function https://www.sqlite.org/lang_corefunc.html#sqlite_compileoption_used
     */
    template<class X>
    constexpr internal::builtin_function_t<int, internal::sqlite_compileoption_used_string, X>
    sqlite_compileoption_used(X option) {
        return {std::tuple<X>{std::forward<X>(option)}};
    }

    /**
     *  SQLITE_COMPILEOPTION_GET(N) function https://www.sqlite.org/lang_corefunc.html#sqlite_compileoption_get
     */
    template<class N>
    constexpr internal::builtin_function_t<std::unique_ptr<std::string>, internal::sqlite_compileoption_get_string, N>
    sqlite_compileoption_get(N n) {
        return {std::tuple<N>{std::forward<N>(n)}};
    }
#endif
#ifdef SQLITE_ENABLE_OFFSET_SQL_FUNC
    /**
     *  SQLITE_OFFSET(X) function https://www.sqlite.org/lang_corefunc.html#sqlite_offset
     *
     *  Only available if the linked SQLite is compiled with SQLITE_ENABLE_OFFSET_SQL_FUNC.
     */
    template<class C>
    constexpr internal::builtin_function_t<std::unique_ptr<int64>, internal::sqlite_offset_string, C>
    sqlite_offset(C column) {
        static_assert(polyfill::disjunction<std::is_member_pointer<C>, internal::is_column_pointer<C>>::value,
                      "sqlite_offset() argument must be a column");
        return {std::tuple<C>{std::forward<C>(column)}};
    }
#endif
    /**
     *  COALESCE(X,Y,...) function https://www.sqlite.org/lang_corefunc.html#coalesce
     */
    template<class R = void, class... Args>
    constexpr internal::builtin_function_t<std::conditional_t<std::is_void_v<R>, internal::common_argument_type<>, R>,
                                           internal::coalesce_string,
                                           Args...>
    coalesce(Args... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  IFNULL(X,Y) function https://www.sqlite.org/lang_corefunc.html#ifnull
     */
    template<class R = void, class X, class Y>
    constexpr internal::builtin_function_t<
        std::conditional_t<std::is_void_v<R>, internal::common_argument_type<0, 1>, R>,
        internal::ifnull_string,
        X,
        Y>
    ifnull(X x, Y y) {
        return {std::make_tuple(std::move(x), std::move(y))};
    }

    /**
     *  NULLIF(X,Y) using common return type of X and Y
     */
    template<class R = void, class X, class Y>
    constexpr internal::builtin_function_t<
        std::conditional_t<std::is_void_v<R>, std::optional<internal::common_argument_type<0, 1>>, R>,
        internal::nullif_string,
        X,
        Y>
    nullif(X x, Y y) {
        return {std::make_tuple(std::move(x), std::move(y))};
    }

    /**
     *  ZEROBLOB(N) function https://www.sqlite.org/lang_corefunc.html#zeroblob
     */
    template<class N>
    constexpr internal::builtin_function_t<std::vector<char>, internal::zeroblob_string, N> zeroblob(N n) {
        return {std::tuple<N>{std::forward<N>(n)}};
    }

    /**
     *  SUBSTR(X,Y) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::substr_string, X, Y> substr(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  SUBSTR(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y, class Z>
    constexpr internal::builtin_function_t<std::string, internal::substr_string, X, Y, Z> substr(X x, Y y, Z z) {
        return {std::tuple<X, Y, Z>{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)}};
    }

#if SQLITE_VERSION_NUMBER >= 3034000
    /**
     *  SUBSTRING(X,Y) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::substring_string, X, Y> substring(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  SUBSTRING(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y, class Z>
    constexpr internal::builtin_function_t<std::string, internal::substring_string, X, Y, Z> substring(X x, Y y, Z z) {
        return {std::tuple<X, Y, Z>{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)}};
    }
#endif
#ifdef SQLITE_SOUNDEX
    /**
     *  SOUNDEX(X) function https://www.sqlite.org/lang_corefunc.html#soundex
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::soundex_string, X> soundex(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008001
    /**
     *  LIKELIHOOD(X,Y) function https://www.sqlite.org/lang_corefunc.html#likelihood
     *
     *  The probability is stored as a literal, never as a bound parameter:
     *  SQLite requires the second argument to be a floating point constant between 0.0 and 1.0,
     *  and rejects a statement that binds it.
     */
    template<class X>
    constexpr internal::
        builtin_function_t<internal::argument<0>, internal::likelihood_string, X, internal::literal_holder<double>>
        likelihood(X x, double probability) {
#ifdef SQLITE_ORM_CPP20_IS_CONSTANT_EVALUATED_SUPPORTED
        //  a probability outside [0.0, 1.0] makes SQLite reject the statement at prepare time;
        //  when the call is constant-evaluated the error surfaces right here, at compile time
        if (std::is_constant_evaluated() && !(probability >= 0.0 && probability <= 1.0)) {
            throw std::domain_error("likelihood() probability must be a constant between 0.0 and 1.0");
        }
#endif
        return {std::tuple<X, internal::literal_holder<double>>{std::forward<X>(x), {probability}}};
    }

    /**
     *  UNLIKELY(X) function https://www.sqlite.org/lang_corefunc.html#unlikely
     */
    template<class X>
    constexpr internal::builtin_function_t<internal::argument<0>, internal::unlikely_string, X> unlikely(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    /**
     *  PRINTF(FORMAT,...) function https://www.sqlite.org/lang_corefunc.html#printf
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::printf_string, Args...> printf(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008006
    /**
     *  LIKELY(X) function https://www.sqlite.org/lang_corefunc.html#likely
     */
    template<class X>
    constexpr internal::builtin_function_t<internal::argument<0>, internal::likely_string, X> likely(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3032000
    /**
     *  IIF(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#iif
     *
     *  The return type is the common type of Y and Z, unless it is explicitly specified as a template argument.
     *  The function is only enabled if a common type of Y and Z can be determined or the return type is explicit.
     *
     *  Example:
     *
     *  auto rows = storage.select(iif<std::string>(c(&User::age) > 18, "adult", "minor"));
     */
    template<class R = void, class X, class Y, class Z>
    constexpr internal::builtin_function_t<
        std::conditional_t<std::is_void_v<R>, internal::common_argument_type<1, 2>, R>,
        internal::iif_string,
        X,
        Y,
        Z>
    iif(X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3035000
    /**
     *  SIGN(X) function https://www.sqlite.org/lang_corefunc.html#sign
     *
     *  The return type defaults to `int`; any other bindable type such as `std::optional<int>` can be specified
     *  explicitly as a template argument, which is handy when NULL is a possible result (e.g. for a non-numeric argument).
     */
    template<class R = int, class X>
    constexpr internal::builtin_function_t<R, internal::sign_string, X> sign(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3038000
    /**
     *  FORMAT(FORMAT,...) function https://www.sqlite.org/lang_corefunc.html#format
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::format_string, Args...> format(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3041000
    /**
     *  UNHEX(X) function https://www.sqlite.org/lang_corefunc.html#unhex
     */
    template<class X>
    constexpr internal::builtin_function_t<std::vector<char>, internal::unhex_string, X> unhex(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  UNHEX(X,Y) function https://www.sqlite.org/lang_corefunc.html#unhex
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::vector<char>, internal::unhex_string, X, Y> unhex(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    /**
     *  OCTET_LENGTH(X) function https://www.sqlite.org/lang_corefunc.html#octet_length
     */
    template<class X>
    constexpr internal::builtin_function_t<int, internal::octet_length_string, X> octet_length(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3044000
    /**
     *  CONCAT(X,...) function https://www.sqlite.org/lang_corefunc.html#concat
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::concat_string, Args...> concat(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    /**
     *  CONCAT_WS(SEP,X,...) function https://www.sqlite.org/lang_corefunc.html#concat_ws
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::concat_ws_string, Args...> concat_ws(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3048000
    /**
     *  IF(X,Y,Z) function, an alias for IIF() https://www.sqlite.org/lang_corefunc.html#iif
     *
     *  The return type is the common type of Y and Z, unless it is explicitly specified as a template argument.
     *  The function is only enabled if a common type of Y and Z can be determined or the return type is explicit.
     */
    template<class R = void, class X, class Y, class Z>
    constexpr internal::builtin_function_t<
        std::conditional_t<std::is_void_v<R>, internal::common_argument_type<1, 2>, R>,
        internal::if_string,
        X,
        Y,
        Z>
    if_(X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3050000
    /**
     *  UNISTR(X) function https://www.sqlite.org/lang_corefunc.html#unistr
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::unistr_string, X> unistr(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  UNISTR_QUOTE(X) function https://www.sqlite.org/lang_corefunc.html#unistr_quote
     */
    template<class X>
    constexpr internal::builtin_function_t<std::string, internal::unistr_quote_string, X> unistr_quote(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
#endif
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in core function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace - as copies, or wrapped by a function template
     *  where the public function takes the return type as a template argument or checks its arguments.
     */
    inline constexpr auto typeof_ = "TYPEOF"_builtin.scalar<std::string(anything)>();
    inline constexpr auto unicode = "UNICODE"_builtin.scalar<int(std::string_view)>();
    inline constexpr auto length = "LENGTH"_builtin.scalar<int(anything)>();
    inline constexpr auto abs = "ABS"_builtin.scalar<std::unique_ptr<double>(anything)>();
    inline constexpr auto lower = "LOWER"_builtin.scalar<std::string(std::string_view)>();
    inline constexpr auto upper = "UPPER"_builtin.scalar<std::string(std::string_view)>();
    inline constexpr auto last_insert_rowid = "LAST_INSERT_ROWID"_builtin.scalar<int64()>();
    inline constexpr auto total_changes = "TOTAL_CHANGES"_builtin.scalar<int()>();
    inline constexpr auto changes = "CHANGES"_builtin.scalar<int()>();
    inline constexpr auto trim =
        "TRIM"_builtin.scalar<std::string(std::string_view), std::string(std::string_view, std::string_view)>();
    inline constexpr auto ltrim =
        "LTRIM"_builtin.scalar<std::string(std::string_view), std::string(std::string_view, std::string_view)>();
    inline constexpr auto rtrim =
        "RTRIM"_builtin.scalar<std::string(std::string_view), std::string(std::string_view, std::string_view)>();
    inline constexpr auto hex = "HEX"_builtin.scalar<std::string(anything)>();
    inline constexpr auto quote = "QUOTE"_builtin.scalar<std::string(anything)>();
    inline constexpr auto randomblob = "RANDOMBLOB"_builtin.scalar<std::vector<char>(int)>();
    inline constexpr auto instr = "INSTR"_builtin.scalar<int(anything, anything)>();
    inline constexpr auto replace =
        "REPLACE"_builtin.scalar<std::string(std::string_view, std::string_view, std::string_view)>();
    inline constexpr auto round = "ROUND"_builtin.scalar<double(double), double(double, int)>();
#if SQLITE_VERSION_NUMBER >= 3007016
    inline constexpr auto char_ = "CHAR"_builtin.scalar<std::string(variadic<int>)>();
    inline constexpr auto random = "RANDOM"_builtin.scalar<int()>();
#endif
    inline constexpr auto sqlite_version = "SQLITE_VERSION"_builtin.scalar<std::string()>();
    inline constexpr auto sqlite_source_id = "SQLITE_SOURCE_ID"_builtin.scalar<std::string()>();
#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    inline constexpr auto sqlite_compileoption_used =
        "SQLITE_COMPILEOPTION_USED"_builtin.scalar<int(std::string_view)>();
    inline constexpr auto sqlite_compileoption_get =
        "SQLITE_COMPILEOPTION_GET"_builtin.scalar<std::unique_ptr<std::string>(int)>();
#endif
#ifdef SQLITE_ENABLE_OFFSET_SQL_FUNC
    inline constexpr auto sqlite_offset = "SQLITE_OFFSET"_builtin.scalar<std::unique_ptr<int64>(anything)>();
#endif
    inline constexpr auto coalesce =
        "COALESCE"_builtin.scalar<common_argument_type<>(anything, anything, variadic<anything>)>();
    inline constexpr auto ifnull = "IFNULL"_builtin.scalar<common_argument_type<0, 1>(anything, anything)>();
    inline constexpr auto nullif =
        "NULLIF"_builtin.scalar<std::optional<common_argument_type<0, 1>>(anything, anything)>();
    inline constexpr auto zeroblob = "ZEROBLOB"_builtin.scalar<std::vector<char>(int)>();
    inline constexpr auto substr =
        "SUBSTR"_builtin.scalar<std::string(std::string_view, int), std::string(std::string_view, int, int)>();
#if SQLITE_VERSION_NUMBER >= 3034000
    inline constexpr auto substring =
        "SUBSTRING"_builtin.scalar<std::string(std::string_view, int), std::string(std::string_view, int, int)>();
#endif
#ifdef SQLITE_SOUNDEX
    inline constexpr auto soundex = "SOUNDEX"_builtin.scalar<std::string(std::string_view)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3008001
    inline constexpr auto likelihood = "LIKELIHOOD"_builtin.scalar<argument<0>(anything, double)>();
    inline constexpr auto unlikely = "UNLIKELY"_builtin.scalar<argument<0>(anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    inline constexpr auto printf = "PRINTF"_builtin.scalar<std::string(std::string_view, variadic<anything>)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3008006
    inline constexpr auto likely = "LIKELY"_builtin.scalar<argument<0>(anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3032000
    inline constexpr auto iif = "IIF"_builtin.scalar<common_argument_type<1, 2>(anything, anything, anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3035000
    inline constexpr auto sign = "SIGN"_builtin.scalar<int(double)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3038000
    inline constexpr auto format = "FORMAT"_builtin.scalar<std::string(std::string_view, variadic<anything>)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3041000
    inline constexpr auto unhex =
        "UNHEX"_builtin
            .scalar<std::vector<char>(std::string_view), std::vector<char>(std::string_view, std::string_view)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    inline constexpr auto octet_length = "OCTET_LENGTH"_builtin.scalar<int(anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3044000
    inline constexpr auto concat = "CONCAT"_builtin.scalar<std::string(anything, variadic<anything>)>();
    inline constexpr auto concat_ws =
        "CONCAT_WS"_builtin.scalar<std::string(std::string_view, anything, variadic<anything>)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3048000
    inline constexpr auto if_ = "IF"_builtin.scalar<common_argument_type<1, 2>(anything, anything, anything)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3050000
    inline constexpr auto unistr = "UNISTR"_builtin.scalar<std::string(std::string_view)>();
    inline constexpr auto unistr_quote = "UNISTR_QUOTE"_builtin.scalar<std::string(std::string_view)>();
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  TYPEOF(x) function https://sqlite.org/lang_corefunc.html#typeof
     */
    inline constexpr orm_builtin_function auto typeof_ = internal::typeof_;

    /**
     *  UNICODE(x) function https://sqlite.org/lang_corefunc.html#unicode
     */
    inline constexpr orm_builtin_function auto unicode = internal::unicode;

    /**
     *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
     */
    inline constexpr orm_builtin_function auto length = internal::length;

    /**
     *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
     */
    template<class X>
    constexpr auto abs(X x) {
        return internal::abs(std::move(x));
    }

    /**
     *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
     */
    inline constexpr orm_builtin_function auto lower = internal::lower;

    /**
     *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
     */
    inline constexpr orm_builtin_function auto upper = internal::upper;

    /**
     *  LAST_INSERT_ROWID() function https://www.sqlite.org/lang_corefunc.html#last_insert_rowid
     */
    inline constexpr orm_builtin_function auto last_insert_rowid = internal::last_insert_rowid;

    /**
     *  TOTAL_CHANGES() function https://sqlite.org/lang_corefunc.html#total_changes
     */
    inline constexpr orm_builtin_function auto total_changes = internal::total_changes;

    /**
     *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
     */
    inline constexpr orm_builtin_function auto changes = internal::changes;

    /**
     *  TRIM(X) and TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
     */
    inline constexpr orm_builtin_function auto trim = internal::trim;

    /**
     *  LTRIM(X) and LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    inline constexpr orm_builtin_function auto ltrim = internal::ltrim;

    /**
     *  RTRIM(X) and RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    inline constexpr orm_builtin_function auto rtrim = internal::rtrim;

    /**
     *  HEX(X) function https://sqlite.org/lang_corefunc.html#hex
     */
    inline constexpr orm_builtin_function auto hex = internal::hex;

    /**
     *  QUOTE(X) function https://sqlite.org/lang_corefunc.html#quote
     */
    inline constexpr orm_builtin_function auto quote = internal::quote;

    /**
     *  RANDOMBLOB(X) function https://sqlite.org/lang_corefunc.html#randomblob
     */
    inline constexpr orm_builtin_function auto randomblob = internal::randomblob;

    /**
     *  INSTR(X) function https://sqlite.org/lang_corefunc.html#instr
     */
    inline constexpr orm_builtin_function auto instr = internal::instr;

    /**
     *  REPLACE(X) function https://sqlite.org/lang_corefunc.html#replace
     */
    template<class X, class Y, class Z>
        requires (internal::count_tuple<std::tuple<X, Y, Z>, internal::is_into>::value == 0)
    constexpr auto replace(X x, Y y, Z z) {
        return internal::replace(std::move(x), std::move(y), std::move(z));
    }

    /**
     *  ROUND(X) and ROUND(X,Y) function https://sqlite.org/lang_corefunc.html#round
     */
    template<class... Args>
        requires requires(Args... args) { internal::round(std::move(args)...); }
    constexpr auto round(Args... args) {
        return internal::round(std::move(args)...);
    }

#if SQLITE_VERSION_NUMBER >= 3007016
    /**
     *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
     */
    inline constexpr orm_builtin_function auto char_ = internal::char_;

    /**
     *  RANDOM() function https://www.sqlite.org/lang_corefunc.html#random
     */
    constexpr auto random() {
        return internal::random();
    }
#endif
    /**
     *  SQLITE_VERSION() function https://www.sqlite.org/lang_corefunc.html#sqlite_version
     */
    inline constexpr orm_builtin_function auto sqlite_version = internal::sqlite_version;

    /**
     *  SQLITE_SOURCE_ID() function https://www.sqlite.org/lang_corefunc.html#sqlite_source_id
     */
    inline constexpr orm_builtin_function auto sqlite_source_id = internal::sqlite_source_id;

#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    /**
     *  SQLITE_COMPILEOPTION_USED(X) function https://www.sqlite.org/lang_corefunc.html#sqlite_compileoption_used
     */
    inline constexpr orm_builtin_function auto sqlite_compileoption_used = internal::sqlite_compileoption_used;

    /**
     *  SQLITE_COMPILEOPTION_GET(N) function https://www.sqlite.org/lang_corefunc.html#sqlite_compileoption_get
     */
    inline constexpr orm_builtin_function auto sqlite_compileoption_get = internal::sqlite_compileoption_get;
#endif
#ifdef SQLITE_ENABLE_OFFSET_SQL_FUNC
    /**
     *  SQLITE_OFFSET(X) function https://www.sqlite.org/lang_corefunc.html#sqlite_offset
     *
     *  Only available if the linked SQLite is compiled with SQLITE_ENABLE_OFFSET_SQL_FUNC.
     */
    template<class C>
    constexpr auto sqlite_offset(C column) {
        static_assert(polyfill::disjunction<std::is_member_pointer<C>, internal::is_column_pointer<C>>::value,
                      "sqlite_offset() argument must be a column");
        return internal::sqlite_offset(std::move(column));
    }
#endif
    /**
     *  COALESCE(X,Y,...) function https://www.sqlite.org/lang_corefunc.html#coalesce
     */
    template<class R = void, class... Args>
    constexpr auto coalesce(Args... args) {
        return internal::coalesce.template operator()<R>(std::move(args)...);
    }

    /**
     *  IFNULL(X,Y) function https://www.sqlite.org/lang_corefunc.html#ifnull
     */
    template<class R = void, class X, class Y>
    constexpr auto ifnull(X x, Y y) {
        return internal::ifnull.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  NULLIF(X,Y) using common return type of X and Y
     */
    template<class R = void, class X, class Y>
    constexpr auto nullif(X x, Y y) {
        return internal::nullif.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  ZEROBLOB(N) function https://www.sqlite.org/lang_corefunc.html#zeroblob
     */
    inline constexpr orm_builtin_function auto zeroblob = internal::zeroblob;

    /**
     *  SUBSTR(X,Y) and SUBSTR(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    inline constexpr orm_builtin_function auto substr = internal::substr;

#if SQLITE_VERSION_NUMBER >= 3034000
    /**
     *  SUBSTRING(X,Y) and SUBSTRING(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    inline constexpr orm_builtin_function auto substring = internal::substring;
#endif
#ifdef SQLITE_SOUNDEX
    /**
     *  SOUNDEX(X) function https://www.sqlite.org/lang_corefunc.html#soundex
     */
    inline constexpr orm_builtin_function auto soundex = internal::soundex;
#endif
#if SQLITE_VERSION_NUMBER >= 3008001
    /**
     *  LIKELIHOOD(X,Y) function https://www.sqlite.org/lang_corefunc.html#likelihood
     *
     *  The probability is stored as a literal, never as a bound parameter:
     *  SQLite requires the second argument to be a floating point constant between 0.0 and 1.0,
     *  and rejects a statement that binds it.
     */
    template<class X>
    constexpr auto likelihood(X x, double probability) {
#ifdef SQLITE_ORM_CPP20_IS_CONSTANT_EVALUATED_SUPPORTED
        //  a probability outside [0.0, 1.0] makes SQLite reject the statement at prepare time;
        //  when the call is constant-evaluated the error surfaces right here, at compile time
        if (std::is_constant_evaluated() && !(probability >= 0.0 && probability <= 1.0)) {
            throw std::domain_error("likelihood() probability must be a constant between 0.0 and 1.0");
        }
#endif
        return internal::likelihood(std::move(x), internal::literal_holder<double>{probability});
    }

    /**
     *  UNLIKELY(X) function https://www.sqlite.org/lang_corefunc.html#unlikely
     */
    inline constexpr orm_builtin_function auto unlikely = internal::unlikely;
#endif
#if SQLITE_VERSION_NUMBER >= 3008003
    /**
     *  PRINTF(FORMAT,...) function https://www.sqlite.org/lang_corefunc.html#printf
     */
    template<class... Args>
        requires requires(Args... args) { internal::printf(std::move(args)...); }
    constexpr auto printf(Args... args) {
        return internal::printf(std::move(args)...);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008006
    /**
     *  LIKELY(X) function https://www.sqlite.org/lang_corefunc.html#likely
     */
    inline constexpr orm_builtin_function auto likely = internal::likely;
#endif
#if SQLITE_VERSION_NUMBER >= 3032000
    /**
     *  IIF(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#iif
     *
     *  The return type is the common type of Y and Z, unless it is explicitly specified as a template argument.
     *  The function is only enabled if a common type of Y and Z can be determined or the return type is explicit.
     *
     *  Example:
     *
     *  auto rows = storage.select(iif<std::string>(c(&User::age) > 18, "adult", "minor"));
     */
    template<class R = void, class X, class Y, class Z>
    constexpr auto iif(X x, Y y, Z z) {
        return internal::iif.template operator()<R>(std::move(x), std::move(y), std::move(z));
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3035000
    /**
     *  SIGN(X) function https://www.sqlite.org/lang_corefunc.html#sign
     *
     *  The return type defaults to `int`; any other bindable type such as `std::optional<int>` can be specified
     *  explicitly as a template argument, which is handy when NULL is a possible result (e.g. for a non-numeric argument).
     */
    template<class R = int, class X>
    constexpr auto sign(X x) {
        return internal::sign.template operator()<R>(std::move(x));
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3038000
    /**
     *  FORMAT(FORMAT,...) function https://www.sqlite.org/lang_corefunc.html#format
     */
    inline constexpr orm_builtin_function auto format = internal::format;
#endif
#if SQLITE_VERSION_NUMBER >= 3041000
    /**
     *  UNHEX(X) and UNHEX(X,Y) function https://www.sqlite.org/lang_corefunc.html#unhex
     */
    inline constexpr orm_builtin_function auto unhex = internal::unhex;
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    /**
     *  OCTET_LENGTH(X) function https://www.sqlite.org/lang_corefunc.html#octet_length
     */
    inline constexpr orm_builtin_function auto octet_length = internal::octet_length;
#endif
#if SQLITE_VERSION_NUMBER >= 3044000
    /**
     *  CONCAT(X,...) function https://www.sqlite.org/lang_corefunc.html#concat
     */
    inline constexpr orm_builtin_function auto concat = internal::concat;

    /**
     *  CONCAT_WS(SEP,X,...) function https://www.sqlite.org/lang_corefunc.html#concat_ws
     */
    inline constexpr orm_builtin_function auto concat_ws = internal::concat_ws;
#endif
#if SQLITE_VERSION_NUMBER >= 3048000
    /**
     *  IF(X,Y,Z) function, an alias for IIF() https://www.sqlite.org/lang_corefunc.html#iif
     *
     *  The return type is the common type of Y and Z, unless it is explicitly specified as a template argument.
     *  The function is only enabled if a common type of Y and Z can be determined or the return type is explicit.
     */
    template<class R = void, class X, class Y, class Z>
    constexpr auto if_(X x, Y y, Z z) {
        return internal::if_.template operator()<R>(std::move(x), std::move(y), std::move(z));
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3050000
    /**
     *  UNISTR(X) function https://www.sqlite.org/lang_corefunc.html#unistr
     */
    inline constexpr orm_builtin_function auto unistr = internal::unistr;

    /**
     *  UNISTR_QUOTE(X) function https://www.sqlite.org/lang_corefunc.html#unistr_quote
     */
    inline constexpr orm_builtin_function auto unistr_quote = internal::unistr_quote;
#endif
}
#endif
