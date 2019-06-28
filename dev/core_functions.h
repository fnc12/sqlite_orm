#pragma once

#include <string>   //  std::string
#include <tuple>    //  std::make_tuple, std::tuple_size
#include <type_traits>  //  std::forward, std::is_base_of, std::enable_if
#include <memory>   //  std::unique_ptr
#include <vector>   //  std::vector

#include "conditions.h"
#include "operators.h"
#include "is_base_of_template.h"

namespace sqlite_orm {
    
    namespace core_functions {
        
        /**
         *  Base class for operator overloading
         *  R - return type
         *  S - class with operator std::string
         *  Args - function arguments types
         */
        template<class R, class S, class ...Args>
        struct core_function_t : S, internal::arithmetic_t {
            using return_type = R;
            using string_type = S;
            using args_type = std::tuple<Args...>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            core_function_t(args_type &&args_) : args(std::move(args_)) {}
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
        
        struct date_string {
            operator std::string() const {
                return "DATE";
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
    }
    
    /**
     *  Cute operators for core functions
     */
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::lesser_than_t<F, R> operator<(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::lesser_or_equal_t<F, R> operator<=(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::greater_than_t<F, R> operator>(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::greater_or_equal_t<F, R> operator>=(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::is_equal_t<F, R> operator==(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    template<
    class F,
    class R,
    typename = typename std::enable_if<internal::is_base_of_template<F, core_functions::core_function_t>::value>::type>
    conditions::is_not_equal_t<F, R> operator!=(F f, R r) {
        return {std::move(f), std::move(r)};
    }
    
    /**
     *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
     */
    template<class T>
    core_functions::core_function_t<int, core_functions::length_string, T> length(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    /**
     *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
     */
    template<class T>
    core_functions::core_function_t<std::unique_ptr<double>, core_functions::abs_string, T> abs(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    /**
     *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
     */
    template<class T>
    core_functions::core_function_t<std::string, core_functions::lower_string, T> lower(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    /**
     *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
     */
    template<class T>
    core_functions::core_function_t<std::string, core_functions::upper_string, T> upper(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    /**
     *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
     */
    inline core_functions::core_function_t<int, core_functions::changes_string> changes() {
        return {{}};
    }
    
    /**
     *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class T>
    core_functions::core_function_t<std::string, core_functions::trim_string, T> trim(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    /**
     *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
     */
    template<class X, class Y>
    core_functions::core_function_t<std::string, core_functions::trim_string, X, Y> trim(X &&x, Y &&y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {std::move(args)};
    }
    
    /**
     *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X>
    core_functions::core_function_t<std::string, core_functions::ltrim_string, X> ltrim(X &&x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {std::move(args)};
    }
    
    /**
     *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
     */
    template<class X, class Y>
    core_functions::core_function_t<std::string, core_functions::ltrim_string, X, Y> ltrim(X &&x, Y &&y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {std::move(args)};
    }
    
    /**
     *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X>
    core_functions::core_function_t<std::string, core_functions::rtrim_string, X> rtrim(X &&x) {
        std::tuple<X> args{std::forward<X>(x)};
        return {std::move(args)};
    }
    
    /**
     *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
     */
    template<class X, class Y>
    core_functions::core_function_t<std::string, core_functions::rtrim_string, X, Y> rtrim(X &&x, Y &&y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {std::move(args)};
    }
    
#if SQLITE_VERSION_NUMBER >= 3007016
    
    /**
     *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
     */
    template<class ...Args>
    core_functions::core_function_t<std::string, core_functions::char_string, Args...> char_(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    /**
     *  RANDOM() function https://www.sqlite.org/lang_corefunc.html#random
     */
    inline core_functions::core_function_t<int, core_functions::random_string> random() {
        return {{}};
    }
    
#endif
    
    /**
     *  COALESCE(X,Y,...) function https://www.sqlite.org/lang_corefunc.html#coalesce
     */
    template<class R, class ...Args>
    core_functions::core_function_t<R, core_functions::coalesce_string, Args...> coalesce(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    /**
     *  DATE(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class ...Args>
    core_functions::core_function_t<std::string, core_functions::date_string, Args...> date(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
    /**
     *  DATETIME(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class ...Args>
    core_functions::core_function_t<std::string, core_functions::datetime_string, Args...> datetime(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
    /**
     *  JULIANDAY(timestring, modifier, modifier, ...) function https://www.sqlite.org/lang_datefunc.html
     */
    template<class ...Args>
    core_functions::core_function_t<double, core_functions::julianday_string, Args...> julianday(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
    /**
     *  ZEROBLOB(N) function https://www.sqlite.org/lang_corefunc.html#zeroblob
     */
    template<class N>
    core_functions::core_function_t<std::vector<char>, core_functions::zeroblob_string, N> zeroblob(N &&n) {
        std::tuple<N> args{std::forward<N>(n)};
        return {std::move(args)};
    }
    
    /**
     *  SUBSTR(X,Y) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y>
    core_functions::core_function_t<std::string, core_functions::substr_string, X, Y> substr(X &&x, Y &&y) {
        std::tuple<X, Y> args{std::forward<X>(x), std::forward<Y>(y)};
        return {std::move(args)};
    }
    
    /**
     *  SUBSTR(X,Y,Z) function https://www.sqlite.org/lang_corefunc.html#substr
     */
    template<class X, class Y, class Z>
    core_functions::core_function_t<std::string, core_functions::substr_string, X, Y, Z> substr(X &&x, Y &&y, Z &&z) {
        std::tuple<X, Y, Z> args{std::forward<X>(x), std::forward<Y>(y), std::forward<Z>(z)};
        return {std::move(args)};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value + std::is_base_of<internal::arithmetic_t, R>::value > 0)>::type>
    internal::add_t<L, R> operator+(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value + std::is_base_of<internal::arithmetic_t, R>::value > 0)>::type>
    internal::sub_t<L, R> operator-(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value + std::is_base_of<internal::arithmetic_t, R>::value > 0)>::type>
    internal::mul_t<L, R> operator*(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value + std::is_base_of<internal::arithmetic_t, R>::value > 0)>::type>
    internal::div_t<L, R> operator/(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<(std::is_base_of<internal::arithmetic_t, L>::value + std::is_base_of<internal::arithmetic_t, R>::value > 0)>::type>
    internal::mod_t<L, R> operator%(L l, R r) {
        return {std::move(l), std::move(r)};
    }
}
