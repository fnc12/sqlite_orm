#pragma once

#include <string>  //  std::string
#include <tuple>  //  std::make_tuple, std::tuple_size
#include <type_traits>  //  std::forward, std::is_base_of, std::enable_if
#include <memory>  //  std::unique_ptr
#include <vector>  //  std::vector

#include "conditions.h"
#include "operators.h"
#include "is_base_of_template.h"

namespace sqlite_orm {

    using int64 = sqlite_int64;
    using uint64 = sqlite_uint64;

    namespace internal {

        template<class T>
        struct unique_ptr_result_of {};

        /**
         *  Base class for operator overloading
         *  R - return type
         *  S - class with operator std::string
         *  Args - function arguments types
         */
        template<class R, class S, class... Args>
        struct core_function_t : S, internal::arithmetic_t {
            using return_type = R;
            using string_type = S;
            using args_type = std::tuple<Args...>;

            static constexpr const size_t args_size = std::tuple_size<args_type>::value;

            args_type args;

            core_function_t(args_type&& args_) : args(std::move(args_)) {}
        };

        struct typeof_string {
            operator std::string() const {
                return "TYPEOF";
            }
        };

        struct unicode_string {
            operator std::string() const {
                return "UNICODE";
            }
        };

        struct length_string {
            operator std::string() const {
                return "LENGTH";
            }
        };

        struct abs_string {
            operator std::string() const {
                return "ABS";
            }
        };

        struct lower_string {
            operator std::string() const {
                return "LOWER";
            }
        };

        struct upper_string {
            operator std::string() const {
                return "UPPER";
            }
        };

        struct last_insert_rowid_string {
            operator std::string() const {
                return "LAST_INSERT_ROWID";
            }
        };

        struct total_changes_string {
            operator std::string() const {
                return "TOTAL_CHANGES";
            }
        };

        struct changes_string {
            operator std::string() const {
                return "CHANGES";
            }
        };

        struct trim_string {
            operator std::string() const {
                return "TRIM";
            }
        };

        struct ltrim_string {
            operator std::string() const {
                return "LTRIM";
            }
        };

        struct rtrim_string {
            operator std::string() const {
                return "RTRIM";
            }
        };

        struct hex_string {
            operator std::string() const {
                return "HEX";
            }
        };

        struct quote_string {
            operator std::string() const {
                return "QUOTE";
            }
        };

        struct randomblob_string {
            operator std::string() const {
                return "RANDOMBLOB";
            }
        };

        struct instr_string {
            operator std::string() const {
                return "INSTR";
            }
        };

        struct replace_string {
            operator std::string() const {
                return "REPLACE";
            }
        };

        struct round_string {
            operator std::string() const {
                return "ROUND";
            }
        };

#if SQLITE_VERSION_NUMBER >= 3007016

        struct char_string {
            operator std::string() const {
                return "CHAR";
            }
        };

        struct random_string {
            operator std::string() const {
                return "RANDOM";
            }
        };

#endif

        struct coalesce_string {
            operator std::string() const {
                return "COALESCE";
            }
        };

        struct ifnull_string {
            operator std::string() const {
                return "IFNULL";
            }
        };

        struct date_string {
            operator std::string() const {
                return "DATE";
            }
        };

        struct time_string {
            operator std::string() const {
                return "TIME";
            }
        };

        struct datetime_string {
            operator std::string() const {
                return "DATETIME";
            }
        };

        struct julianday_string {
            operator std::string() const {
                return "JULIANDAY";
            }
        };

        struct strftime_string {
            operator std::string() const {
                return "STRFTIME";
            }
        };

        struct zeroblob_string {
            operator std::string() const {
                return "ZEROBLOB";
            }
        };

        struct substr_string {
            operator std::string() const {
                return "SUBSTR";
            }
        };
#ifdef SQLITE_SOUNDEX
        struct soundex_string {
            operator std::string() const {
                return "SOUNDEX";
            }
        };
#endif
        struct total_string {
            operator std::string() const {
                return "TOTAL";
            }
        };

        struct sum_string {
            operator std::string() const {
                return "SUM";
            }
        };

        struct count_string {
            operator std::string() const {
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
        struct count_asterisk_without_type : count_string {};

        struct avg_string {
            operator std::string() const {
                return "AVG";
            }
        };

        struct max_string {
            operator std::string() const {
                return "MAX";
            }
        };

        struct min_string {
            operator std::string() const {
                return "MIN";
            }
        };

        struct group_concat_string {
            operator std::string() const {
                return "GROUP_CONCAT";
            }
        };
#ifdef SQLITE_ENABLE_JSON1
        struct json_string {
            operator std::string() const {
                return "JSON";
            }
        };

        struct json_array_string {
            operator std::string() const {
                return "JSON_ARRAY";
            }
        };

        struct json_array_length_string {
            operator std::string() const {
                return "JSON_ARRAY_LENGTH";
            }
        };

        struct json_extract_string {
            operator std::string() const {
                return "JSON_EXTRACT";
            }
        };

        struct json_insert_string {
            operator std::string() const {
                return "JSON_INSERT";
            }
        };

        struct json_replace_string {
            operator std::string() const {
                return "JSON_REPLACE";
            }
        };

        struct json_set_string {
            operator std::string() const {
                return "JSON_SET";
            }
        };

        struct json_object_string {
            operator std::string() const {
                return "JSON_OBJECT";
            }
        };

        struct json_patch_string {
            operator std::string() const {
                return "JSON_PATCH";
            }
        };

        struct json_remove_string {
            operator std::string() const {
                return "JSON_REMOVE";
            }
        };

        struct json_type_string {
            operator std::string() const {
                return "JSON_TYPE";
            }
        };

        struct json_valid_string {
            operator std::string() const {
                return "JSON_VALID";
            }
        };

        struct json_quote_string {
            operator std::string() const {
                return "JSON_QUOTE";
            }
        };

        struct json_group_array_string {
            operator std::string() const {
                return "JSON_GROUP_ARRAY";
            }
        };

        struct json_group_object_string {
            operator std::string() const {
                return "JSON_GROUP_OBJECT";
            }
        };
#endif  //  SQLITE_ENABLE_JSON1
    }

    /**
     *  Cute operators for core functions
     */

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::lesser_than_t<F, R> operator<(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::lesser_or_equal_t<F, R> operator<=(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::greater_than_t<F, R> operator>(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::greater_or_equal_t<F, R> operator>=(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::is_equal_t<F, R> operator==(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    template<
        class F,
        class R,
        typename = typename std::enable_if<internal::is_base_of_template<F, internal::core_function_t>::value>::type>
    internal::is_not_equal_t<F, R> operator!=(F f, R r) {
        return {std::move(f), std::move(r)};
    }

    /**
     *  TYPEOF(x) function https://sqlite.org/lang_corefunc.html#typeof
     */
    template<class T>
    internal::core_function_t<std::string, internal::typeof_string, T> typeof_(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  UNICODE(x) function https://sqlite.org/lang_corefunc.html#unicode
     */
    template<class T>
    internal::core_function_t<int, internal::unicode_string, T> unicode(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
     */
    template<class T>
    internal::core_function_t<int, internal::length_string, T> length(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
     */
    template<class T>
    internal::core_function_t<std::unique_ptr<double>, internal::abs_string, T> abs(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
     */
    template<class T>
    internal::core_function_t<std::string, internal::lower_string, T> lower(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
     */
    template<class T>
    internal::core_function_t<std::string, internal::upper_string, T> upper(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  LAST_INSERT_ROWID(x) function https://www.sqlite.org/lang_corefunc.html#last_insert_rowid
     */
    inline internal::core_function_t<int64, internal::last_insert_rowid_string> last_insert_rowid() {
        return {{}};
    }

    /**
     *  TOTAL_CHANGES() function https://sqlite.org/lang_corefunc.html#total_changes
     */
    inline internal::core_function_t<int, internal::total_changes_string> total_changes() {
        return {{}};
    }

    /**
     *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
     */
    inline internal::core_function_t<int, internal::changes_string> changes() {
        return {{}};
    }

    /**
     *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class T>
    internal::core_function_t<std::string, internal::trim_string, T> trim(T t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {move(args)};
    }

    /**
     *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class X, class Y>
    internal::core_function_t<std::string, internal::trim_string, X, Y> trim(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    /**
     *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X>
    internal::core_function_t<std::string, internal::ltrim_string, X> ltrim(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X, class Y>
    internal::core_function_t<std::string, internal::ltrim_string, X, Y> ltrim(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    /**
     *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X>
    internal::core_function_t<std::string, internal::rtrim_string, X> rtrim(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X, class Y>
    internal::core_function_t<std::string, internal::rtrim_string, X, Y> rtrim(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    /**
     *  HEX(X) function https://sqlite.org/lang_corefunc.html#hex
     */
    template<class X>
    internal::core_function_t<std::string, internal::hex_string, X> hex(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  QUOTE(X) function https://sqlite.org/lang_corefunc.html#quote
     */
    template<class X>
    internal::core_function_t<std::string, internal::quote_string, X> quote(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  RANDOMBLOB(X) function https://sqlite.org/lang_corefunc.html#randomblob
     */
    template<class X>
    internal::core_function_t<std::vector<char>, internal::randomblob_string, X> randomblob(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  INSTR(X) function https://sqlite.org/lang_corefunc.html#instr
     */
    template<class X, class Y>
    internal::core_function_t<int, internal::instr_string, X, Y> instr(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    /**
     *  REPLACE(X) function https://sqlite.org/lang_corefunc.html#replace
     */
    template<class X, class Y, class Z>
    internal::core_function_t<std::string, internal::replace_string, X, Y, Z> replace(X x, Y y, Z z) {
        std::tuple<X, Y, Z> args{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)};
        return {move(args)};
    }

    /**
     *  ROUND(X) function https://sqlite.org/lang_corefunc.html#round
     */
    template<class X>
    internal::core_function_t<double, internal::round_string, X> round(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  ROUND(X, Y) function https://sqlite.org/lang_corefunc.html#round
     */
    template<class X, class Y>
    internal::core_function_t<double, internal::round_string, X, Y> round(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

#if SQLITE_VERSION_NUMBER >= 3007016

    /**
     *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
     */
    template<class... Args>
    internal::core_function_t<std::string, internal::char_string, Args...> char_(Args... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  RANDOM() function https://www.sqlite.org/lang_corefunc.html#random
     */
    inline internal::core_function_t<int, internal::random_string> random() {
        return {{}};
    }

#endif

    /**
     *  COALESCE(X,Y,...) function https://www.sqlite.org/lang_corefunc.html#coalesce
     */
    template<class R, class... Args>
    internal::core_function_t<R, internal::coalesce_string, Args...> coalesce(Args... args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }

    /**
     *  IFNULL(X,Y) function https://www.sqlite.org/lang_corefunc.html#ifnull
     */
    template<class R, class X, class Y>
    internal::core_function_t<R, internal::ifnull_string, X, Y> ifnull(X x, Y y) {
        return {std::make_tuple(std::move(x), std::move(y))};
    }

    /**
     *  DATE(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    internal::core_function_t<std::string, internal::date_string, Args...> date(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    /**
     *  TIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    internal::core_function_t<std::string, internal::time_string, Args...> time(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    /**
     *  DATETIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    internal::core_function_t<std::string, internal::datetime_string, Args...> datetime(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    /**
     *  JULIANDAY(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    internal::core_function_t<double, internal::julianday_string, Args...> julianday(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    /**
     *  STRFTIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class... Args>
    internal::core_function_t<std::string, internal::strftime_string, Args...> strftime(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    /**
     *  ZEROBLOB(N) function https://www.sqlite.org/lang_corefunc.html#zeroblob
     */
    template<class N>
    internal::core_function_t<std::vector<char>, internal::zeroblob_string, N> zeroblob(N n) {
        std::tuple<N> args{std::forward<N>(n)};
        return {move(args)};
    }

    /**
     *  SUBSTR(X,Y) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y>
    internal::core_function_t<std::string, internal::substr_string, X, Y> substr(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    /**
     *  SUBSTR(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y, class Z>
    internal::core_function_t<std::string, internal::substr_string, X, Y, Z> substr(X x, Y y, Z z) {
        std::tuple<X, Y, Z> args{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)};
        return {move(args)};
    }

#ifdef SQLITE_SOUNDEX
    /**
     *  SOUNDEX(X) function https://www.sqlite.org/lang_corefunc.html#soundex
     */
    template<class X>
    internal::core_function_t<std::string, internal::soundex_string, X> soundex(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }
#endif

    /**
     *  TOTAL(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<double, internal::total_string, X> total(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  SUM(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<std::unique_ptr<double>, internal::sum_string, X> sum(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  COUNT(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<int, internal::count_string, X> count(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  COUNT(*) without FROM function.
     */
    inline internal::count_asterisk_without_type count() {
        return {};
    }

    /**
     *  COUNT(*) with FROM function. Specified type T will be serializeed as
     *  a from argument.
     */
    template<class T>
    internal::count_asterisk_t<T> count() {
        return {};
    }

    /**
     *  AVG(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<double, internal::avg_string, X> avg(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  MAX(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<internal::unique_ptr_result_of<X>, internal::max_string, X> max(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  MIN(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<internal::unique_ptr_result_of<X>, internal::min_string, X> min(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  GROUP_CONCAT(X) aggregate function.
     */
    template<class X>
    internal::core_function_t<std::string, internal::group_concat_string, X> group_concat(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    /**
     *  GROUP_CONCAT(X, Y) aggregate function.
     */
    template<class X, class Y>
    internal::core_function_t<std::string, internal::group_concat_string, X, Y> group_concat(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }
#ifdef SQLITE_ENABLE_JSON1
    template<class X>
    internal::core_function_t<std::string, internal::json_string, X> json(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class... Args>
    internal::core_function_t<std::string, internal::json_array_string, Args...> json_array(Args... args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X>
    internal::core_function_t<int, internal::json_array_length_string, X> json_array_length(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class R, class X>
    internal::core_function_t<R, internal::json_array_length_string, X> json_array_length(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class X, class Y>
    internal::core_function_t<int, internal::json_array_length_string, X, Y> json_array_length(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    template<class R, class X, class Y>
    internal::core_function_t<R, internal::json_array_length_string, X, Y> json_array_length(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    template<class R, class X, class... Args>
    internal::core_function_t<R, internal::json_extract_string, X, Args...> json_extract(X x, Args... args) {
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X, class... Args>
    internal::core_function_t<std::string, internal::json_insert_string, X, Args...> json_insert(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_insert must be odd");
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X, class... Args>
    internal::core_function_t<std::string, internal::json_replace_string, X, Args...> json_replace(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_replace must be odd");
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X, class... Args>
    internal::core_function_t<std::string, internal::json_set_string, X, Args...> json_set(X x, Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_set must be odd");
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class... Args>
    internal::core_function_t<std::string, internal::json_object_string, Args...> json_object(Args... args) {
        static_assert(std::tuple_size<std::tuple<Args...>>::value % 2 == 0,
                      "number of arguments in json_object must be even");
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X, class Y>
    internal::core_function_t<std::string, internal::json_patch_string, X, Y> json_patch(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    template<class X, class... Args>
    internal::core_function_t<std::string, internal::json_remove_string, X, Args...> json_remove(X x, Args... args) {
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class R, class X, class... Args>
    internal::core_function_t<R, internal::json_remove_string, X, Args...> json_remove(X x, Args... args) {
        std::tuple<X, Args...> t{std::forward<X>(x), std::forward<Args>(args)...};
        return {move(t)};
    }

    template<class X>
    internal::core_function_t<std::string, internal::json_type_string, X> json_type(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class R, class X>
    internal::core_function_t<R, internal::json_type_string, X> json_type(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class X, class Y>
    internal::core_function_t<std::string, internal::json_type_string, X, Y> json_type(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    template<class R, class X, class Y>
    internal::core_function_t<R, internal::json_type_string, X, Y> json_type(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

    template<class X>
    internal::core_function_t<bool, internal::json_valid_string, X> json_valid(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class R, class X>
    internal::core_function_t<R, internal::json_quote_string, X> json_quote(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class X>
    internal::core_function_t<std::string, internal::json_group_array_string, X> json_group_array(X x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {move(args)};
    }

    template<class X, class Y>
    internal::core_function_t<std::string, internal::json_group_object_string, X, Y> json_group_object(X x, Y y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {move(args)};
    }

#endif  //  SQLITE_ENABLE_JSON1
    template<class L,
             class R,
             typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value +
                                                     std::is_base_of<internal::arithmetic_t, R>::value >
                                                 0)>::type>
    internal::add_t<L, R> operator+(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L,
             class R,
             typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value +
                                                     std::is_base_of<internal::arithmetic_t, R>::value >
                                                 0)>::type>
    internal::sub_t<L, R> operator-(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L,
             class R,
             typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value +
                                                     std::is_base_of<internal::arithmetic_t, R>::value >
                                                 0)>::type>
    internal::mul_t<L, R> operator*(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L,
             class R,
             typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value +
                                                     std::is_base_of<internal::arithmetic_t, R>::value >
                                                 0)>::type>
    internal::div_t<L, R> operator/(L l, R r) {
        return {std::move(l), std::move(r)};
    }

    template<class L,
             class R,
             typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value +
                                                     std::is_base_of<internal::arithmetic_t, R>::value >
                                                 0)>::type>
    internal::mod_t<L, R> operator%(L l, R r) {
        return {std::move(l), std::move(r)};
    }
}
