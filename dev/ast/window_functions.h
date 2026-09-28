#pragma once

/** @file The built-in window functions.
 *
 *        They are no grammar family of their own - each is a function in its own right, and it is applied
 *        only as the function of an OVER clause. Hence they get no node trait of their own: the consumers of
 *        the OVER node handle them, telling them apart from the aggregate functions an OVER clause may apply
 *        as well by the semantic trait `is_builtin_window_function`. To that end each of them declares its
 *        SQL name, its return type - which may use the return type placeholders of
 *        `vocabulary/algorithms/argument_placeholders.h` - and the tuple of its call arguments.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same, std::disjunction
#include <tuple>  //  std::tuple
#include <string_view>  //  std::string_view
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/node_algorithms.h"  //  argument
#include "../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits
#include "window.h"  //  over_t, validate_over_arguments

namespace sqlite_orm::internal {
    struct row_number_t {
        static constexpr std::string_view name = "ROW_NUMBER";
        using return_type = int;
        using args_tuple = std::tuple<>;

        SQLITE_ORM_NOUNIQUEADDRESS args_tuple args;

        template<class... OverArgs>
        over_t<row_number_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    /*
     *  Note: `rank_t` carries a second meaning on its own, outside of an OVER clause: as the bare `rank` it
     *  serves the deprecated `order_by(rank())` spelling of the hidden FTS5 rank column, and goes with it in v1.11.
     */
    struct rank_t {
        static constexpr std::string_view name = "rank";
        using return_type = int;
        using args_tuple = std::tuple<>;

        SQLITE_ORM_NOUNIQUEADDRESS args_tuple args;

        template<class... OverArgs>
        over_t<rank_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct dense_rank_t {
        static constexpr std::string_view name = "DENSE_RANK";
        using return_type = int;
        using args_tuple = std::tuple<>;

        SQLITE_ORM_NOUNIQUEADDRESS args_tuple args;

        template<class... OverArgs>
        over_t<dense_rank_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct percent_rank_t {
        static constexpr std::string_view name = "PERCENT_RANK";
        using return_type = double;
        using args_tuple = std::tuple<>;

        SQLITE_ORM_NOUNIQUEADDRESS args_tuple args;

        template<class... OverArgs>
        over_t<percent_rank_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct cume_dist_t {
        static constexpr std::string_view name = "CUME_DIST";
        using return_type = double;
        using args_tuple = std::tuple<>;

        SQLITE_ORM_NOUNIQUEADDRESS args_tuple args;

        template<class... OverArgs>
        over_t<cume_dist_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct ntile_t {
        static constexpr std::string_view name = "NTILE";
        using return_type = int;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<ntile_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct lag_t {
        static constexpr std::string_view name = "LAG";
        using return_type = argument<0>;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<lag_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct lead_t {
        static constexpr std::string_view name = "LEAD";
        using return_type = argument<0>;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<lead_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct first_value_t {
        static constexpr std::string_view name = "FIRST_VALUE";
        using return_type = argument<0>;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<first_value_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct last_value_t {
        static constexpr std::string_view name = "LAST_VALUE";
        using return_type = argument<0>;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<last_value_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct nth_value_t {
        static constexpr std::string_view name = "NTH_VALUE";
        using return_type = argument<0>;
        using args_tuple = std::tuple<Args...>;

        args_tuple args;

        template<class... OverArgs>
        over_t<nth_value_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class T>
    constexpr bool is_builtin_window_function_v =
        std::disjunction<std::is_same<T, row_number_t>,
                         std::is_same<T, rank_t>,
                         std::is_same<T, dense_rank_t>,
                         std::is_same<T, percent_rank_t>,
                         std::is_same<T, cume_dist_t>,
                         polyfill::is_specialization_of<T, ntile_t>,
                         polyfill::is_specialization_of<T, lag_t>,
                         polyfill::is_specialization_of<T, lead_t>,
                         polyfill::is_specialization_of<T, first_value_t>,
                         polyfill::is_specialization_of<T, last_value_t>,
                         polyfill::is_specialization_of<T, nth_value_t>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  ROW_NUMBER() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::row_number_t row_number() {
        return {};
    }

    /**
     *  RANK() window function
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::rank_t rank() {
        return {};
    }

    /**
     *  DENSE_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::dense_rank_t dense_rank() {
        return {};
    }

    /**
     *  PERCENT_RANK() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::percent_rank_t percent_rank() {
        return {};
    }

    /**
     *  CUME_DIST() window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    inline internal::cume_dist_t cume_dist() {
        return {};
    }

    /**
     *  NTILE(N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class N>
    internal::ntile_t<N> ntile(N n) {
        return {std::tuple<N>{std::forward<N>(n)}};
    }

    /**
     *  LAG(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::lag_t<E> lag(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LAG(expr, offset) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O>
    internal::lag_t<E, O> lag(E expression, O offset) {
        return {{std::forward<E>(expression), std::forward<O>(offset)}};
    }

    /**
     *  LAG(expr, offset, default) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O, class D>
    internal::lag_t<E, O, D> lag(E expression, O offset, D defaultValue) {
        return {{std::forward<E>(expression), std::forward<O>(offset), std::forward<D>(defaultValue)}};
    }

    /**
     *  LEAD(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::lead_t<E> lead(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LEAD(expr, offset) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O>
    internal::lead_t<E, O> lead(E expression, O offset) {
        return {{std::forward<E>(expression), std::forward<O>(offset)}};
    }

    /**
     *  LEAD(expr, offset, default) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class O, class D>
    internal::lead_t<E, O, D> lead(E expression, O offset, D defaultValue) {
        return {{std::forward<E>(expression), std::forward<O>(offset), std::forward<D>(defaultValue)}};
    }

    /**
     *  FIRST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::first_value_t<E> first_value(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  LAST_VALUE(expr) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E>
    internal::last_value_t<E> last_value(E expression) {
        return {std::tuple<E>{std::forward<E>(expression)}};
    }

    /**
     *  NTH_VALUE(expr, N) window function.
     *  https://sqlite.org/windowfunctions.html#built-in_window_functions
     */
    template<class E, class N>
    internal::nth_value_t<E, N> nth_value(E expression, N n) {
        return {{std::forward<E>(expression), std::forward<N>(n)}};
    }
}
