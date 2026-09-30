#pragma once

/** @file The built-in window functions: ROW_NUMBER(), RANK(), DENSE_RANK(), PERCENT_RANK(), CUME_DIST(),
 *        NTILE(N), LAG(expr), LEAD(expr), FIRST_VALUE(expr), LAST_VALUE(expr) and NTH_VALUE(expr, N).
 *
 *        They are built-in functions of the window kind, applied by an OVER clause: in C++20 builds definition
 *        objects of `ast/builtin_function.h` (`"ROW_NUMBER"_builtin.window<int()>()`), in C++17 builds legacy
 *        factories over `builtin_window_function_t`. Either way their call nodes are built-in function calls,
 *        classified by `is_builtin_function_call` like those of the scalar and aggregate functions.
 *
 *        Like `core.h` for those, this header holds definitions, not nodes, hence it lives outside `ast/`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward
#endif

#include "../../vocabulary/node_algorithms.h"  //  argument
#include "../../ast/builtin_function.h"

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in window function nodes.
     */
    struct row_number_string {
        std::string_view serialize() const {
            return "ROW_NUMBER";
        }
    };

    struct rank_string {
        std::string_view serialize() const {
            return "RANK";
        }
    };

    struct dense_rank_string {
        std::string_view serialize() const {
            return "DENSE_RANK";
        }
    };

    struct percent_rank_string {
        std::string_view serialize() const {
            return "PERCENT_RANK";
        }
    };

    struct cume_dist_string {
        std::string_view serialize() const {
            return "CUME_DIST";
        }
    };

    struct ntile_string {
        std::string_view serialize() const {
            return "NTILE";
        }
    };

    struct lag_string {
        std::string_view serialize() const {
            return "LAG";
        }
    };

    struct lead_string {
        std::string_view serialize() const {
            return "LEAD";
        }
    };

    struct first_value_string {
        std::string_view serialize() const {
            return "FIRST_VALUE";
        }
    };

    struct last_value_string {
        std::string_view serialize() const {
            return "LAST_VALUE";
        }
    };

    struct nth_value_string {
        std::string_view serialize() const {
            return "NTH_VALUE";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  ROW_NUMBER() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::builtin_window_function_t<int, internal::row_number_string> row_number() {
        return {std::tuple<>{}};
    }

    /**
     *  RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::builtin_window_function_t<int, internal::rank_string> rank() {
        return {std::tuple<>{}};
    }

    /**
     *  DENSE_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::builtin_window_function_t<int, internal::dense_rank_string> dense_rank() {
        return {std::tuple<>{}};
    }

    /**
     *  PERCENT_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::builtin_window_function_t<double, internal::percent_rank_string> percent_rank() {
        return {std::tuple<>{}};
    }

    /**
     *  CUME_DIST() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::builtin_window_function_t<double, internal::cume_dist_string> cume_dist() {
        return {std::tuple<>{}};
    }

    /**
     *  NTILE(N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class N>
    internal::builtin_window_function_t<int, internal::ntile_string, N> ntile(N n) {
        return {std::tuple<N>{std::forward<N>(n)}};
    }

    /**
     *  LAG(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::builtin_window_function_t<internal::argument<0>, internal::lag_string, E> lag(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LAG(expr, offset) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O>
    internal::builtin_window_function_t<internal::argument<0>, internal::lag_string, E, O> lag(E expression, O offset) {
        return {std::tuple<E, O>{std::forward<E>(expression), std::forward<O>(offset)}};
    }

    /**
     *  LAG(expr, offset, default) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O, class D>
    internal::builtin_window_function_t<internal::argument<0>, internal::lag_string, E, O, D>
    lag(E expression, O offset, D defaultValue) {
        return {
            std::tuple<E, O, D>{std::forward<E>(expression), std::forward<O>(offset), std::forward<D>(defaultValue)}};
    }

    /**
     *  LEAD(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::builtin_window_function_t<internal::argument<0>, internal::lead_string, E> lead(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LEAD(expr, offset) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O>
    internal::builtin_window_function_t<internal::argument<0>, internal::lead_string, E, O> lead(E expression,
                                                                                                 O offset) {
        return {std::tuple<E, O>{std::forward<E>(expression), std::forward<O>(offset)}};
    }

    /**
     *  LEAD(expr, offset, default) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O, class D>
    internal::builtin_window_function_t<internal::argument<0>, internal::lead_string, E, O, D>
    lead(E expression, O offset, D defaultValue) {
        return {
            std::tuple<E, O, D>{std::forward<E>(expression), std::forward<O>(offset), std::forward<D>(defaultValue)}};
    }

    /**
     *  FIRST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::builtin_window_function_t<internal::argument<0>, internal::first_value_string, E>
    first_value(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LAST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::builtin_window_function_t<internal::argument<0>, internal::last_value_string, E>
    last_value(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  NTH_VALUE(expr, N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class N>
    internal::builtin_window_function_t<internal::argument<0>, internal::nth_value_string, E, N> nth_value(E expression,
                                                                                                           N n) {
        return {std::tuple<E, N>{std::forward<E>(expression), std::forward<N>(n)}};
    }
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in window function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace as copies.
     */
    inline constexpr auto row_number = "ROW_NUMBER"_builtin.window<int()>();
    inline constexpr auto rank = "RANK"_builtin.window<int()>();
    inline constexpr auto dense_rank = "DENSE_RANK"_builtin.window<int()>();
    inline constexpr auto percent_rank = "PERCENT_RANK"_builtin.window<double()>();
    inline constexpr auto cume_dist = "CUME_DIST"_builtin.window<double()>();
    inline constexpr auto ntile = "NTILE"_builtin.window<int(anything)>();
    inline constexpr auto lag = "LAG"_builtin.window<argument<0>(anything),
                                                     argument<0>(anything, anything),
                                                     argument<0>(anything, anything, anything)>();
    inline constexpr auto lead = "LEAD"_builtin.window<argument<0>(anything),
                                                       argument<0>(anything, anything),
                                                       argument<0>(anything, anything, anything)>();
    inline constexpr auto first_value = "FIRST_VALUE"_builtin.window<argument<0>(anything)>();
    inline constexpr auto last_value = "LAST_VALUE"_builtin.window<argument<0>(anything)>();
    inline constexpr auto nth_value = "NTH_VALUE"_builtin.window<argument<0>(anything, anything)>();
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  ROW_NUMBER() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto row_number = internal::row_number;

    /**
     *  RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto rank = internal::rank;

    /**
     *  DENSE_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto dense_rank = internal::dense_rank;

    /**
     *  PERCENT_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto percent_rank = internal::percent_rank;

    /**
     *  CUME_DIST() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto cume_dist = internal::cume_dist;

    /**
     *  NTILE(N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto ntile = internal::ntile;

    /**
     *  LAG(expr[, offset[, default]]) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto lag = internal::lag;

    /**
     *  LEAD(expr[, offset[, default]]) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto lead = internal::lead;

    /**
     *  FIRST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto first_value = internal::first_value;

    /**
     *  LAST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto last_value = internal::last_value;

    /**
     *  NTH_VALUE(expr, N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline constexpr orm_builtin_function auto nth_value = internal::nth_value;
}
#endif
