#pragma once

/** @file The built-in mathematical functions https://www.sqlite.org/lang_mathfunc.html,
 *        available if SQLite is compiled with SQLITE_ENABLE_MATH_FUNCTIONS.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple
#include <utility>  //  std::move, std::forward
#include <string_view>  //  std::string_view
#endif

#include "../../ast/builtin_function.h"

#ifdef SQLITE_ENABLE_MATH_FUNCTIONS

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in mathematical function nodes.
     */
    struct acos_string {
        std::string_view serialize() const {
            return "ACOS";
        }
    };

    struct acosh_string {
        std::string_view serialize() const {
            return "ACOSH";
        }
    };

    struct asin_string {
        std::string_view serialize() const {
            return "ASIN";
        }
    };

    struct asinh_string {
        std::string_view serialize() const {
            return "ASINH";
        }
    };

    struct atan_string {
        std::string_view serialize() const {
            return "ATAN";
        }
    };

    struct atan2_string {
        std::string_view serialize() const {
            return "ATAN2";
        }
    };

    struct atanh_string {
        std::string_view serialize() const {
            return "ATANH";
        }
    };

    struct ceil_string {
        std::string_view serialize() const {
            return "CEIL";
        }
    };

    struct ceiling_string {
        std::string_view serialize() const {
            return "CEILING";
        }
    };

    struct cos_string {
        std::string_view serialize() const {
            return "COS";
        }
    };

    struct cosh_string {
        std::string_view serialize() const {
            return "COSH";
        }
    };

    struct degrees_string {
        std::string_view serialize() const {
            return "DEGREES";
        }
    };

    struct exp_string {
        std::string_view serialize() const {
            return "EXP";
        }
    };

    struct floor_string {
        std::string_view serialize() const {
            return "FLOOR";
        }
    };

    struct ln_string {
        std::string_view serialize() const {
            return "LN";
        }
    };

    struct log_string {
        std::string_view serialize() const {
            return "LOG";
        }
    };

    struct log10_string {
        std::string_view serialize() const {
            return "LOG10";
        }
    };

    struct log2_string {
        std::string_view serialize() const {
            return "LOG2";
        }
    };

    struct mod_string {
        std::string_view serialize() const {
            return "MOD";
        }
    };

    struct pi_string {
        std::string_view serialize() const {
            return "PI";
        }
    };

    struct pow_string {
        std::string_view serialize() const {
            return "POW";
        }
    };

    struct power_string {
        std::string_view serialize() const {
            return "POWER";
        }
    };

    struct radians_string {
        std::string_view serialize() const {
            return "RADIANS";
        }
    };

    struct sin_string {
        std::string_view serialize() const {
            return "SIN";
        }
    };

    struct sinh_string {
        std::string_view serialize() const {
            return "SINH";
        }
    };

    struct sqrt_string {
        std::string_view serialize() const {
            return "SQRT";
        }
    };

    struct tan_string {
        std::string_view serialize() const {
            return "TAN";
        }
    };

    struct tanh_string {
        std::string_view serialize() const {
            return "TANH";
        }
    };

    struct trunc_string {
        std::string_view serialize() const {
            return "TRUNC";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  ACOS(X) function https://www.sqlite.org/lang_mathfunc.html#acos
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::acos(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::acos<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::acos_string, X> acos(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ACOSH(X) function https://www.sqlite.org/lang_mathfunc.html#acosh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::acosh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::acosh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::acosh_string, X> acosh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ASIN(X) function https://www.sqlite.org/lang_mathfunc.html#asin
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::asin(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::asin<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::asin_string, X> asin(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ASINH(X) function https://www.sqlite.org/lang_mathfunc.html#asinh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::asinh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::asinh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::asinh_string, X> asinh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ATAN(X) function https://www.sqlite.org/lang_mathfunc.html#atan
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atan(1));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atan<std::optional<double>>(1));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::atan_string, X> atan(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  ATAN2(X, Y) function https://www.sqlite.org/lang_mathfunc.html#atan2
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atan2(1, 3));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atan2<std::optional<double>>(1, 3));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::atan2_string, X, Y> atan2(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  ATANH(X) function https://www.sqlite.org/lang_mathfunc.html#atanh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atanh(1));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atanh<std::optional<double>>(1));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::atanh_string, X> atanh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  CEIL(X) function https://www.sqlite.org/lang_mathfunc.html#ceil
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ceil(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ceil<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::ceil_string, X> ceil(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  CEILING(X) function https://www.sqlite.org/lang_mathfunc.html#ceil
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ceiling(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ceiling<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::ceiling_string, X> ceiling(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  COS(X) function https://www.sqlite.org/lang_mathfunc.html#cos
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::cos(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::cos<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::cos_string, X> cos(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  COSH(X)  function https://www.sqlite.org/lang_mathfunc.html#cosh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::cosh(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::cosh<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::cosh_string, X> cosh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  DEGREES(X) function https://www.sqlite.org/lang_mathfunc.html#degrees
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::degrees(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::degrees<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::degrees_string, X> degrees(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  EXP(X) function https://www.sqlite.org/lang_mathfunc.html#exp
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::exp(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::exp<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::exp_string, X> exp(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  FLOOR(X) function https://www.sqlite.org/lang_mathfunc.html#floor
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::floor(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::floor<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::floor_string, X> floor(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  LN(X) function https://www.sqlite.org/lang_mathfunc.html#ln
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ln(200));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ln<std::optional<double>>(200));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::ln_string, X> ln(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  LOG(X) function https://www.sqlite.org/lang_mathfunc.html#log
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log(100));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log<std::optional<double>>(100));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::log_string, X> log(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  LOG10(X) function https://www.sqlite.org/lang_mathfunc.html#log
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log10(100));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log10<std::optional<double>>(100));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::log10_string, X> log10(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  LOG(B, X) function https://www.sqlite.org/lang_mathfunc.html#log
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log(10, 100));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log<std::optional<double>>(10, 100));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class B, class X>
    constexpr internal::builtin_function_t<R, internal::log_string, B, X> log(B b, X x) {
        return {std::tuple<B, X>{std::forward<B>(b), std::forward<X>(x)}};
    }

    /**
     *  LOG2(X) function https://www.sqlite.org/lang_mathfunc.html#log2
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log2(64));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log2<std::optional<double>>(64));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::log2_string, X> log2(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  MOD(X, Y) function https://www.sqlite.org/lang_mathfunc.html#mod
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::mod_f(6, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::mod_f<std::optional<double>>(6, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::mod_string, X, Y> mod_f(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  PI() function https://www.sqlite.org/lang_mathfunc.html#pi
     *
     *  The return type defaults to `double`; any other bindable type such as `float` can be specified
     *  explicitly as a template argument.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::pi());   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::pi<float>());   //  decltype(rows) is std::vector<float>
     */
    template<class R = double>
    constexpr internal::builtin_function_t<R, internal::pi_string> pi() {
        return {{}};
    }

    /**
     *  POW(X, Y) function https://www.sqlite.org/lang_mathfunc.html#pow
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::pow(2, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::pow<std::optional<double>>(2, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::pow_string, X, Y> pow(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  POWER(X, Y) function https://www.sqlite.org/lang_mathfunc.html#pow
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::power(2, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::power<std::optional<double>>(2, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr internal::builtin_function_t<R, internal::power_string, X, Y> power(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  RADIANS(X) function https://www.sqlite.org/lang_mathfunc.html#radians
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::radians(&Triangle::cornerAInDegrees));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::radians<std::optional<double>>(&Triangle::cornerAInDegrees));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::radians_string, X> radians(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  SIN(X) function https://www.sqlite.org/lang_mathfunc.html#sin
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sin(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sin<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::sin_string, X> sin(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  SINH(X) function https://www.sqlite.org/lang_mathfunc.html#sinh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sinh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sinh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::sinh_string, X> sinh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  SQRT(X) function https://www.sqlite.org/lang_mathfunc.html#sqrt
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sqrt(25));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sqrt<int>(25));   //  decltype(rows) is std::vector<int>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::sqrt_string, X> sqrt(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  TAN(X) function https://www.sqlite.org/lang_mathfunc.html#tan
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::tan(&Triangle::cornerC));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::tan<float>(&Triangle::cornerC));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::tan_string, X> tan(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  TANH(X) function https://www.sqlite.org/lang_mathfunc.html#tanh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::tanh(&Triangle::cornerC));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::tanh<float>(&Triangle::cornerC));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::tanh_string, X> tanh(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  TRUNC(X) function https://www.sqlite.org/lang_mathfunc.html#trunc
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::trunc(5.5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::trunc<float>(5.5));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr internal::builtin_function_t<R, internal::trunc_string, X> trunc(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in mathematical function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace - as copies, or wrapped by a function template
     *  where the public function takes the return type as a template argument or checks its arguments.
     */
    inline constexpr auto acos = "ACOS"_builtin.scalar<double(double)>();
    inline constexpr auto acosh = "ACOSH"_builtin.scalar<double(double)>();
    inline constexpr auto asin = "ASIN"_builtin.scalar<double(double)>();
    inline constexpr auto asinh = "ASINH"_builtin.scalar<double(double)>();
    inline constexpr auto atan = "ATAN"_builtin.scalar<double(double)>();
    inline constexpr auto atan2 = "ATAN2"_builtin.scalar<double(double, double)>();
    inline constexpr auto atanh = "ATANH"_builtin.scalar<double(double)>();
    inline constexpr auto ceil = "CEIL"_builtin.scalar<double(double)>();
    inline constexpr auto ceiling = "CEILING"_builtin.scalar<double(double)>();
    inline constexpr auto cos = "COS"_builtin.scalar<double(double)>();
    inline constexpr auto cosh = "COSH"_builtin.scalar<double(double)>();
    inline constexpr auto degrees = "DEGREES"_builtin.scalar<double(double)>();
    inline constexpr auto exp = "EXP"_builtin.scalar<double(double)>();
    inline constexpr auto floor = "FLOOR"_builtin.scalar<double(double)>();
    inline constexpr auto ln = "LN"_builtin.scalar<double(double)>();
    inline constexpr auto log = "LOG"_builtin.scalar<double(double), double(double, double)>();
    inline constexpr auto log10 = "LOG10"_builtin.scalar<double(double)>();
    inline constexpr auto log2 = "LOG2"_builtin.scalar<double(double)>();
    inline constexpr auto mod_f = "MOD"_builtin.scalar<double(double, double)>();
    inline constexpr auto pi = "PI"_builtin.scalar<double()>();
    inline constexpr auto pow = "POW"_builtin.scalar<double(double, double)>();
    inline constexpr auto power = "POWER"_builtin.scalar<double(double, double)>();
    inline constexpr auto radians = "RADIANS"_builtin.scalar<double(double)>();
    inline constexpr auto sin = "SIN"_builtin.scalar<double(double)>();
    inline constexpr auto sinh = "SINH"_builtin.scalar<double(double)>();
    inline constexpr auto sqrt = "SQRT"_builtin.scalar<double(double)>();
    inline constexpr auto tan = "TAN"_builtin.scalar<double(double)>();
    inline constexpr auto tanh = "TANH"_builtin.scalar<double(double)>();
    inline constexpr auto trunc = "TRUNC"_builtin.scalar<double(double)>();
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  ACOS(X) function https://www.sqlite.org/lang_mathfunc.html#acos
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::acos(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::acos<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto acos(X x) {
        return internal::acos.template operator()<R>(std::move(x));
    }

    /**
     *  ACOSH(X) function https://www.sqlite.org/lang_mathfunc.html#acosh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::acosh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::acosh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto acosh(X x) {
        return internal::acosh.template operator()<R>(std::move(x));
    }

    /**
     *  ASIN(X) function https://www.sqlite.org/lang_mathfunc.html#asin
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::asin(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::asin<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto asin(X x) {
        return internal::asin.template operator()<R>(std::move(x));
    }

    /**
     *  ASINH(X) function https://www.sqlite.org/lang_mathfunc.html#asinh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::asinh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::asinh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto asinh(X x) {
        return internal::asinh.template operator()<R>(std::move(x));
    }

    /**
     *  ATAN(X) function https://www.sqlite.org/lang_mathfunc.html#atan
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atan(1));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atan<std::optional<double>>(1));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto atan(X x) {
        return internal::atan.template operator()<R>(std::move(x));
    }

    /**
     *  ATAN2(X, Y) function https://www.sqlite.org/lang_mathfunc.html#atan2
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atan2(1, 3));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atan2<std::optional<double>>(1, 3));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr auto atan2(X x, Y y) {
        return internal::atan2.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  ATANH(X) function https://www.sqlite.org/lang_mathfunc.html#atanh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::atanh(1));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::atanh<std::optional<double>>(1));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto atanh(X x) {
        return internal::atanh.template operator()<R>(std::move(x));
    }

    /**
     *  CEIL(X) function https://www.sqlite.org/lang_mathfunc.html#ceil
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ceil(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ceil<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto ceil(X x) {
        return internal::ceil.template operator()<R>(std::move(x));
    }

    /**
     *  CEILING(X) function https://www.sqlite.org/lang_mathfunc.html#ceil
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ceiling(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ceiling<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto ceiling(X x) {
        return internal::ceiling.template operator()<R>(std::move(x));
    }

    /**
     *  COS(X) function https://www.sqlite.org/lang_mathfunc.html#cos
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::cos(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::cos<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto cos(X x) {
        return internal::cos.template operator()<R>(std::move(x));
    }

    /**
     *  COSH(X)  function https://www.sqlite.org/lang_mathfunc.html#cosh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::cosh(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::cosh<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto cosh(X x) {
        return internal::cosh.template operator()<R>(std::move(x));
    }

    /**
     *  DEGREES(X) function https://www.sqlite.org/lang_mathfunc.html#degrees
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::degrees(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::degrees<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto degrees(X x) {
        return internal::degrees.template operator()<R>(std::move(x));
    }

    /**
     *  EXP(X) function https://www.sqlite.org/lang_mathfunc.html#exp
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::exp(&Triangle::cornerB));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::exp<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto exp(X x) {
        return internal::exp.template operator()<R>(std::move(x));
    }

    /**
     *  FLOOR(X) function https://www.sqlite.org/lang_mathfunc.html#floor
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::floor(&User::rating));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::floor<std::optional<double>>(&User::rating));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto floor(X x) {
        return internal::floor.template operator()<R>(std::move(x));
    }

    /**
     *  LN(X) function https://www.sqlite.org/lang_mathfunc.html#ln
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::ln(200));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::ln<std::optional<double>>(200));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto ln(X x) {
        return internal::ln.template operator()<R>(std::move(x));
    }

    /**
     *  LOG(X) and LOG(B,X) function https://www.sqlite.org/lang_mathfunc.html#log
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log(100));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log<std::optional<double>>(100));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class... Args>
    constexpr auto log(Args... args) {
        return internal::log.template operator()<R>(std::move(args)...);
    }

    /**
     *  LOG10(X) function https://www.sqlite.org/lang_mathfunc.html#log
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log10(100));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log10<std::optional<double>>(100));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto log10(X x) {
        return internal::log10.template operator()<R>(std::move(x));
    }

    /**
     *  LOG2(X) function https://www.sqlite.org/lang_mathfunc.html#log2
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::log2(64));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::log2<std::optional<double>>(64));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto log2(X x) {
        return internal::log2.template operator()<R>(std::move(x));
    }

    /**
     *  MOD(X, Y) function https://www.sqlite.org/lang_mathfunc.html#mod
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::mod_f(6, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::mod_f<std::optional<double>>(6, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr auto mod_f(X x, Y y) {
        return internal::mod_f.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  PI() function https://www.sqlite.org/lang_mathfunc.html#pi
     *
     *  The return type defaults to `double`; any other bindable type such as `float` can be specified
     *  explicitly as a template argument.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::pi());   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::pi<float>());   //  decltype(rows) is std::vector<float>
     */
    template<class R = double>
    constexpr auto pi() {
        return internal::pi.template operator()<R>();
    }

    /**
     *  POW(X, Y) function https://www.sqlite.org/lang_mathfunc.html#pow
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::pow(2, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::pow<std::optional<double>>(2, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr auto pow(X x, Y y) {
        return internal::pow.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  POWER(X, Y) function https://www.sqlite.org/lang_mathfunc.html#pow
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::power(2, 5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::power<std::optional<double>>(2, 5));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X, class Y>
    constexpr auto power(X x, Y y) {
        return internal::power.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  RADIANS(X) function https://www.sqlite.org/lang_mathfunc.html#radians
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::radians(&Triangle::cornerAInDegrees));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::radians<std::optional<double>>(&Triangle::cornerAInDegrees));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto radians(X x) {
        return internal::radians.template operator()<R>(std::move(x));
    }

    /**
     *  SIN(X) function https://www.sqlite.org/lang_mathfunc.html#sin
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sin(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sin<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto sin(X x) {
        return internal::sin.template operator()<R>(std::move(x));
    }

    /**
     *  SINH(X) function https://www.sqlite.org/lang_mathfunc.html#sinh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sinh(&Triangle::cornerA));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sinh<std::optional<double>>(&Triangle::cornerA));   //  decltype(rows) is std::vector<std::optional<double>>
     */
    template<class R = double, class X>
    constexpr auto sinh(X x) {
        return internal::sinh.template operator()<R>(std::move(x));
    }

    /**
     *  SQRT(X) function https://www.sqlite.org/lang_mathfunc.html#sqrt
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::sqrt(25));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::sqrt<int>(25));   //  decltype(rows) is std::vector<int>
     */
    template<class R = double, class X>
    constexpr auto sqrt(X x) {
        return internal::sqrt.template operator()<R>(std::move(x));
    }

    /**
     *  TAN(X) function https://www.sqlite.org/lang_mathfunc.html#tan
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::tan(&Triangle::cornerC));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::tan<float>(&Triangle::cornerC));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr auto tan(X x) {
        return internal::tan.template operator()<R>(std::move(x));
    }

    /**
     *  TANH(X) function https://www.sqlite.org/lang_mathfunc.html#tanh
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::tanh(&Triangle::cornerC));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::tanh<float>(&Triangle::cornerC));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr auto tanh(X x) {
        return internal::tanh.template operator()<R>(std::move(x));
    }

    /**
     *  TRUNC(X) function https://www.sqlite.org/lang_mathfunc.html#trunc
     *
     *  The return type defaults to `double`; any other bindable type such as `float` or `std::optional<double>`
     *  can be specified explicitly as a template argument, which is handy when NULL is a possible result.
     *
     *  Examples:
     *
     *  auto rows = storage.select(sqlite_orm::trunc(5.5));   //  decltype(rows) is std::vector<double>
     *  auto rows = storage.select(sqlite_orm::trunc<float>(5.5));   //  decltype(rows) is std::vector<float>
     */
    template<class R = double, class X>
    constexpr auto trunc(X x) {
        return internal::trunc.template operator()<R>(std::move(x));
    }
}
#endif
#endif
