#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple
#include <utility>  //  std::forward, std::move
#endif

#include "ast/window.h"

namespace sqlite_orm::internal {

    struct row_number_t {
        template<class... OverArgs>
        over_t<row_number_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct dense_rank_t {
        template<class... OverArgs>
        over_t<dense_rank_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct percent_rank_t {
        template<class... OverArgs>
        over_t<percent_rank_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct cume_dist_t {
        template<class... OverArgs>
        over_t<cume_dist_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct ntile_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<ntile_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct lag_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<lag_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct lead_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<lead_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct first_value_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<first_value_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct last_value_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<last_value_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class... Args>
    struct nth_value_t {
        using args_type = std::tuple<Args...>;
        args_type args;

        template<class... OverArgs>
        over_t<nth_value_t, OverArgs...> over(OverArgs... overArgs) {
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };
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
