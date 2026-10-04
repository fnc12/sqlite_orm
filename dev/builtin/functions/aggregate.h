#pragma once

/** @file The built-in aggregate functions https://www.sqlite.org/lang_aggfunc.html: COUNT(), TOTAL(), SUM(),
 *        AVG(), MAX(), MIN(), GROUP_CONCAT(), STRING_AGG() and the percentile functions MEDIAN(), PERCENTILE(),
 *        PERCENTILE_CONT() and PERCENTILE_DISC().
 *
 *        MAX() and MIN() take the scalar forms MAX(X,Y,...) and MIN(X,Y,...) along, defined as one overload set
 *        with the aggregate forms.
 *        The aggregate functions of JSON1 are with the other JSON functions, in `json.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple
#include <utility>  //  std::move, std::forward
#include <memory>  //  std::unique_ptr
#include <string_view>  //  std::string_view
#endif

#include "../../alias_traits.h"  //  orm_refers_to_recordset, auto_decay_table_ref_t
#include "../../vocabulary/node_algorithms.h"  //  argument
#include "../../ast/builtin_function.h"  //  count_asterisk_t, count_asterisk_without_type

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  COUNT(*) without FROM function.
     */
    constexpr internal::count_asterisk_without_type count() {
        return {};
    }

    /**
     *  COUNT(*) with FROM function. Specified type T will be serialized as
     *  a from argument.
     */
    template<class T>
    constexpr internal::count_asterisk_t<T> count() {
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  COUNT(*) with FROM function. Specified recordset will be serialized as
     *  a from argument.
     */
    template<orm_refers_to_recordset auto mapped>
    constexpr auto count() {
        return count<internal::auto_decay_table_ref_t<mapped>>();
    }
#endif
}

#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
namespace sqlite_orm::internal {
    /*
     *  The name tags of the legacy built-in aggregate function nodes.
     */
    struct total_string {
        std::string_view serialize() const {
            return "TOTAL";
        }
    };

    struct sum_string {
        std::string_view serialize() const {
            return "SUM";
        }
    };

    struct avg_string {
        std::string_view serialize() const {
            return "AVG";
        }
    };

    struct max_string {
        std::string_view serialize() const {
            return "MAX";
        }
    };

    struct min_string {
        std::string_view serialize() const {
            return "MIN";
        }
    };

    struct group_concat_string {
        std::string_view serialize() const {
            return "GROUP_CONCAT";
        }
    };

#if SQLITE_VERSION_NUMBER >= 3044000
    struct string_agg_string {
        std::string_view serialize() const {
            return "STRING_AGG";
        }
    };
#endif
#ifdef SQLITE_ENABLE_PERCENTILE
    struct median_string {
        std::string_view serialize() const {
            return "MEDIAN";
        }
    };

    struct percentile_string {
        std::string_view serialize() const {
            return "PERCENTILE";
        }
    };

    struct percentile_cont_string {
        std::string_view serialize() const {
            return "PERCENTILE_CONT";
        }
    };

    struct percentile_disc_string {
        std::string_view serialize() const {
            return "PERCENTILE_DISC";
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  TOTAL(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<double, internal::total_string, X> total(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  SUM(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<std::unique_ptr<double>, internal::sum_string, X> sum(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  COUNT(X) aggregate function.
     */
    template<class X>
    internal::builtin_aggregate_function_t<int, internal::count_string, X> count(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  AVG(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<double, internal::avg_string, X> avg(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  MAX(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<std::unique_ptr<internal::argument<0>>, internal::max_string, X>
    max(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  MIN(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<std::unique_ptr<internal::argument<0>>, internal::min_string, X>
    min(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  MAX(X, Y, ...) scalar function.
     *  The return type is the type of the first argument.
     */
    template<class X, class Y, class... Rest>
    constexpr internal::builtin_function_t<std::unique_ptr<internal::argument<0>>, internal::max_string, X, Y, Rest...>
    max(X x, Y y, Rest... rest) {
        return {std::tuple<X, Y, Rest...>{std::forward<X>(x), std::forward<Y>(y), std::forward<Rest>(rest)...}};
    }

    /**
     *  MIN(X, Y, ...) scalar function.
     *  The return type is the type of the first argument.
     */
    template<class X, class Y, class... Rest>
    constexpr internal::builtin_function_t<std::unique_ptr<internal::argument<0>>, internal::min_string, X, Y, Rest...>
    min(X x, Y y, Rest... rest) {
        return {std::tuple<X, Y, Rest...>{std::forward<X>(x), std::forward<Y>(y), std::forward<Rest>(rest)...}};
    }

    /**
     *  GROUP_CONCAT(X) aggregate function.
     */
    template<class X>
    constexpr internal::builtin_aggregate_function_t<std::string, internal::group_concat_string, X> group_concat(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  GROUP_CONCAT(X, Y) aggregate function.
     */
    template<class X, class Y>
    constexpr internal::builtin_aggregate_function_t<std::string, internal::group_concat_string, X, Y>
    group_concat(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

#if SQLITE_VERSION_NUMBER >= 3044000
    /**
     *  STRING_AGG(X,SEP) aggregate function https://www.sqlite.org/lang_aggfunc.html#string_agg
     */
    template<class X, class Y>
    constexpr internal::builtin_aggregate_function_t<std::string, internal::string_agg_string, X, Y> string_agg(X x,
                                                                                                                Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
#ifdef SQLITE_ENABLE_PERCENTILE
    /**
     *  MEDIAN(X) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X>
    constexpr internal::builtin_aggregate_function_t<R, internal::median_string, X> median(X x) {
        return {std::tuple<X>{std::forward<X>(x)}};
    }

    /**
     *  PERCENTILE(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr internal::builtin_aggregate_function_t<R, internal::percentile_string, X, Y> percentile(X x, Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  PERCENTILE_CONT(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr internal::builtin_aggregate_function_t<R, internal::percentile_cont_string, X, Y> percentile_cont(X x,
                                                                                                                Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }

    /**
     *  PERCENTILE_DISC(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr internal::builtin_aggregate_function_t<R, internal::percentile_disc_string, X, Y> percentile_disc(X x,
                                                                                                                Y y) {
        return {std::tuple<X, Y>{std::forward<X>(x), std::forward<Y>(y)}};
    }
#endif
}
#else
namespace sqlite_orm::internal {
    /*
     *  Built-in aggregate function definitions.
     *
     *  Defined here, where the internal `""_builtin` literal is found by unqualified lookup,
     *  and published below in the `sqlite_orm` namespace - as copies, or wrapped by a function template
     *  where the public function takes the return type as a template argument or checks its arguments.
     */
    inline constexpr auto total = "TOTAL"_builtin.aggregate<double(anything)>();
    inline constexpr auto sum = "SUM"_builtin.aggregate<std::unique_ptr<double>(anything)>();
    inline constexpr auto count = "COUNT"_builtin.aggregate<int(anything)>();
    inline constexpr auto avg = "AVG"_builtin.aggregate<double(anything)>();
    // MAX(X) aggregate and MAX(X, Y, ...) scalar: nullable, typed like the first argument
    inline constexpr auto max =
        "MAX"_builtin.function<aggregate_sig<std::unique_ptr<argument<0>>(anything)>,
                               scalar_sig<std::unique_ptr<argument<0>>(anything, anything, variadic<anything>)>>();
    inline constexpr auto min =
        "MIN"_builtin.function<aggregate_sig<std::unique_ptr<argument<0>>(anything)>,
                               scalar_sig<std::unique_ptr<argument<0>>(anything, anything, variadic<anything>)>>();
    inline constexpr auto group_concat =
        "GROUP_CONCAT"_builtin.aggregate<std::string(anything), std::string(anything, std::string_view)>();
#if SQLITE_VERSION_NUMBER >= 3044000
    inline constexpr auto string_agg = "STRING_AGG"_builtin.aggregate<std::string(anything, std::string_view)>();
#endif
#ifdef SQLITE_ENABLE_PERCENTILE
    inline constexpr auto median = "MEDIAN"_builtin.aggregate<std::unique_ptr<double>(anything)>();
    inline constexpr auto percentile = "PERCENTILE"_builtin.aggregate<std::unique_ptr<double>(anything, double)>();
    inline constexpr auto percentile_cont =
        "PERCENTILE_CONT"_builtin.aggregate<std::unique_ptr<double>(anything, double)>();
    inline constexpr auto percentile_disc =
        "PERCENTILE_DISC"_builtin.aggregate<std::unique_ptr<double>(anything, double)>();
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  TOTAL(X) aggregate function.
     */
    inline constexpr orm_builtin_function auto total = internal::total;

    /**
     *  SUM(X) aggregate function.
     */
    inline constexpr orm_builtin_function auto sum = internal::sum;

    /**
     *  COUNT(X) aggregate function.
     */
    template<class X>
    constexpr auto count(X x) {
        return internal::count(std::move(x));
    }

    /**
     *  AVG(X) aggregate function.
     */
    inline constexpr orm_builtin_function auto avg = internal::avg;

    /**
     *  MAX(X) aggregate function and MAX(X,Y,...) scalar function.
     *  https://www.sqlite.org/lang_aggfunc.html#max_agg https://www.sqlite.org/lang_corefunc.html#max_scalar
     */
    inline constexpr orm_builtin_function auto max = internal::max;

    /**
     *  MIN(X) aggregate function and MIN(X,Y,...) scalar function.
     *  https://www.sqlite.org/lang_aggfunc.html#min_agg https://www.sqlite.org/lang_corefunc.html#min_scalar
     */
    inline constexpr orm_builtin_function auto min = internal::min;

    /**
     *  GROUP_CONCAT(X) and GROUP_CONCAT(X,Y) aggregate function.
     */
    inline constexpr orm_builtin_function auto group_concat = internal::group_concat;

#if SQLITE_VERSION_NUMBER >= 3044000
    /**
     *  STRING_AGG(X,SEP) aggregate function https://www.sqlite.org/lang_aggfunc.html#string_agg
     */
    inline constexpr orm_builtin_function auto string_agg = internal::string_agg;
#endif
#ifdef SQLITE_ENABLE_PERCENTILE
    /**
     *  MEDIAN(X) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X>
    constexpr auto median(X x) {
        return internal::median.template operator()<R>(std::move(x));
    }

    /**
     *  PERCENTILE(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr auto percentile(X x, Y y) {
        return internal::percentile.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  PERCENTILE_CONT(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr auto percentile_cont(X x, Y y) {
        return internal::percentile_cont.template operator()<R>(std::move(x), std::move(y));
    }

    /**
     *  PERCENTILE_DISC(Y,P) aggregate function https://www.sqlite.org/lang_aggfunc.html#percentile
     *
     *  The return type defaults to `std::unique_ptr<double>` (the result is NULL for an empty group);
     *  any other bindable type such as `std::optional<double>` can be specified as a template argument.
     */
    template<class R = std::unique_ptr<double>, class X, class Y>
    constexpr auto percentile_disc(X x, Y y) {
        return internal::percentile_disc.template operator()<R>(std::move(x), std::move(y));
    }
#endif
}
#endif
