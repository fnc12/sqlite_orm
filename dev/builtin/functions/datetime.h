#pragma once

/** @file The built-in date and time functions https://www.sqlite.org/lang_datefunc.html: DATE(), TIME(),
 *        DATETIME(), JULIANDAY(), STRFTIME(), UNIXEPOCH() and TIMEDIFF().
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <utility>  //  std::move, std::forward
#include <string_view>  //  std::string_view
#endif

#include "../../sqlite3/sqlite3_types.h"  //  int64
#include "../../ast/builtin_function.h"

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in date and time function nodes.
     */
    struct date_string {
        std::string_view serialize() const {
            return "DATE";
        }
    };

    struct time_string {
        std::string_view serialize() const {
            return "TIME";
        }
    };

    struct datetime_string {
        std::string_view serialize() const {
            return "DATETIME";
        }
    };

    struct julianday_string {
        std::string_view serialize() const {
            return "JULIANDAY";
        }
    };

    struct strftime_string {
        std::string_view serialize() const {
            return "STRFTIME";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3038000
    struct unixepoch_string {
        std::string_view serialize() const {
            return "UNIXEPOCH";
        }
    };
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    struct timediff_string {
        std::string_view serialize() const {
            return "TIMEDIFF";
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  DATE(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::date_string, Args...> date(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    /**
     *  TIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::time_string, Args...> time(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    /**
     *  DATETIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::datetime_string, Args...> datetime(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    /**
     *  JULIANDAY(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<double, internal::julianday_string, Args...> julianday(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

    /**
     *  STRFTIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<std::string, internal::strftime_string, Args...> strftime(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }

#if SQLITE_VERSION_NUMBER >= 3038000
    /**
     *  UNIXEPOCH(timestring, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr internal::builtin_function_t<int64, internal::unixepoch_string, Args...> unixepoch(Args... args) {
        return {std::tuple<Args...>{std::forward<Args>(args)...}};
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    /**
     *  TIMEDIFF(A,B) function https://www.sqlite.org/lang_datefunc.html#tmdiff
     */
    template<class X, class Y>
    constexpr internal::builtin_function_t<std::string, internal::timediff_string, X, Y> timediff(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in date and time function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace - as copies, or wrapped by a function template
     *  where the public function takes the return type as a template argument or checks its arguments.
     */
    inline constexpr auto date = "DATE"_builtin.scalar<std::string(variadic<anything>)>();
    inline constexpr auto time = "TIME"_builtin.scalar<std::string(variadic<anything>)>();
    inline constexpr auto datetime = "DATETIME"_builtin.scalar<std::string(variadic<anything>)>();
    inline constexpr auto julianday = "JULIANDAY"_builtin.scalar<double(variadic<anything>)>();
    inline constexpr auto strftime = "STRFTIME"_builtin.scalar<std::string(std::string_view, variadic<anything>)>();
#if SQLITE_VERSION_NUMBER >= 3038000
    inline constexpr auto unixepoch = "UNIXEPOCH"_builtin.scalar<int64(variadic<anything>)>();
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    inline constexpr auto timediff = "TIMEDIFF"_builtin.scalar<std::string(anything, anything)>();
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  DATE(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    inline constexpr orm_builtin_function auto date = internal::date;

    /**
     *  TIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    constexpr auto time(Args... args) {
        return internal::time(std::move(args)...);
    }

    /**
     *  DATETIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    inline constexpr orm_builtin_function auto datetime = internal::datetime;

    /**
     *  JULIANDAY(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    inline constexpr orm_builtin_function auto julianday = internal::julianday;

    /**
     *  STRFTIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
        requires requires(Args... args) { internal::strftime(std::move(args)...); }
    constexpr auto strftime(Args... args) {
        return internal::strftime(std::move(args)...);
    }

#if SQLITE_VERSION_NUMBER >= 3038000
    /**
     *  UNIXEPOCH(timestring, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    inline constexpr orm_builtin_function auto unixepoch = internal::unixepoch;
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    /**
     *  TIMEDIFF(A,B) function https://www.sqlite.org/lang_datefunc.html#tmdiff
     */
    inline constexpr orm_builtin_function auto timediff = internal::timediff;
#endif
}
#endif
