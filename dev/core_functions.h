#pragma once

#include <string>   //  std::string
#include <tuple>    //  std::make_tuple, std::tuple_size
#include <type_traits>  //  std::forward, std::is_base_of, std::enable_if
#include <memory>   //  std::unique_ptr

#include "conditions.h"
#include "operators.h"
#include "is_base_of_template.h"

namespace sqlite_orm {
    
    namespace core_functions {
        
        /**
         *  Base class for operator overloading
         *  R is return type
         *  S is class with operator std::string
         */
        template<class R, class S>
        struct core_function_t : S, internal::arithmetic_t {
            using return_type = R;
            using string_type = S;
        };
        
        struct length_string {
            operator std::string() const {
                return "LENGTH";
            }
        };
        
        /**
         *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
         */
        template<class T>
        struct length_t : core_function_t<int, length_string> {
            using args_type = std::tuple<T>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            length_t(args_type &&args_) : args(std::move(args_)) {}
        };
        
        struct abs_string {
            operator std::string() const {
                return "ABS";
            }
        };
        
        /**
         *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
         */
        template<class T>
        struct abs_t : core_function_t<std::unique_ptr<double>, abs_string> {
            using args_type = std::tuple<T>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            abs_t(args_type &&args_) : args(std::move(args_)) {}
        };
        
        struct lower_string {
            operator std::string() const {
                return "LOWER";
            }
        };
        
        /**
         *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
         */
        template<class T>
        struct lower_t : core_function_t<std::string, lower_string> {
            using arg_type = T;
            
            arg_type arg;
            
            lower_t(arg_type &&arg_): arg(std::move(arg_)) {}
        };
        
        struct upper_string {
            operator std::string() const {
                return "UPPER";
            }
        };
        
        /**
         *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
         */
        template<class T>
        struct upper_t : core_function_t<std::string, upper_string> {
            using arg_type = T;
            
            arg_type arg;
            
            upper_t(arg_type &&arg_): arg(std::move(arg_)) {}
        };
        
        struct changes_string {
            operator std::string() const {
                return "CHANGES";
            }
        };
        
        /**
         *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
         */
        using changes_t = core_function_t<int, changes_string>;
        
        struct trim_string {
            operator std::string() const {
                return "TRIM";
            }
        };
        
        /**
         *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class T>
        struct trim_single_t : core_function_t<std::string, trim_string> {
            using arg_type = T;
            
            arg_type arg;
            
            trim_single_t(arg_type &&arg_): arg(std::move(arg_)) {}
        };
        
        /**
         *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class X, class Y>
        struct trim_double_t : core_function_t<std::string, trim_string> {
            using args_type = std::tuple<X, Y>;
            
            args_type args;
            
            trim_double_t(X x, Y y): args(std::forward<X>(x), std::forward<Y>(y)) {}
        };
        
        struct ltrim_string {
            operator std::string() const {
                return "LTRIM";
            }
        };
        
        /**
         *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X>
        struct ltrim_single_t : core_function_t<std::string, ltrim_string> {
            using arg_type = X;
            
            arg_type arg;
            
            ltrim_single_t(arg_type &&arg_): arg(std::move(arg_)) {}
        };
        
        /**
         *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X, class Y>
        struct ltrim_double_t : core_function_t<std::string, ltrim_string> {
            using args_type = std::tuple<X, Y>;
            
            args_type args;
            
            ltrim_double_t(X x, Y y): args(std::forward<X>(x), std::forward<Y>(y)) {}
        };
        
        struct rtrim_string {
            operator std::string() const {
                return "RTRIM";
            }
        };
        
        /**
         *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X>
        struct rtrim_single_t : core_function_t<std::string, rtrim_string> {
            using arg_type = X;
            
            arg_type arg;
            
            rtrim_single_t(arg_type &&arg_): arg(std::move(arg_)) {}
        };
        
        /**
         *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X, class Y>
        struct rtrim_double_t : core_function_t<std::string, rtrim_string> {
            using args_type = std::tuple<X, Y>;
            
            args_type args;
            
            rtrim_double_t(X x, Y y): args(std::forward<X>(x), std::forward<Y>(y)) {}
        };
        
        
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        struct char_string {
            operator std::string() const {
                return "CHAR";
            }
        };
        
        /**
         *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
         */
        template<class ...Args>
        struct char_t_ : core_function_t<std::string, char_string> {
            using args_type = std::tuple<Args...>;
            
            args_type args;
            
            char_t_(args_type &&args_): args(std::move(args_)) {}
        };
        
        struct random_string {
            operator std::string() const {
                return "RANDOM";
            }
        };
        
        struct random_t : core_function_t<int, random_string> {};
        
#endif
        
        struct coalesce_string {
            operator std::string() const {
                return "COALESCE";
            }
        };
        
        template<class R, class ...Args>
        struct coalesce_t : core_function_t<R, coalesce_string> {
            using return_type = R;
            using args_type = std::tuple<Args...>;
            
            args_type args;
            
            coalesce_t(args_type args_) : args(std::move(args_)) {}
        };
        
        struct date_string {
            operator std::string() const {
                return "DATE";
            }
        };
        
        template<class ...Args>
        struct date_t : core_function_t<std::string, date_string> {
            using args_type = std::tuple<Args...>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            date_t(args_type &&args_): args(std::move(args_)) {}
        };
        
        struct datetime_string {
            operator std::string() const {
                return "DATETIME";
            }
        };
        
        template<class ...Args>
        struct datetime_t : core_function_t<std::string, datetime_string> {
            using args_type = std::tuple<Args...>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            datetime_t(args_type &&args_): args(std::move(args_)) {}
        };
        
        struct julianday_string {
            operator std::string() const {
                return "JULIANDAY";
            }
        };
        
        template<class ...Args>
        struct julianday_t : core_function_t<double, julianday_string> {
            using args_type = std::tuple<Args...>;
            
            static constexpr const size_t args_size = std::tuple_size<args_type>::value;
            
            args_type args;
            
            julianday_t(args_type &&args_): args(std::move(args_)) {}
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
    
    inline core_functions::random_t random() {
        return {};
    }
    
    template<class ...Args>
    core_functions::date_t<Args...> date(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
    template<class ...Args>
    core_functions::datetime_t<Args...> datetime(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
    template<class ...Args>
    core_functions::julianday_t<Args...> julianday(Args &&...args) {
        std::tuple<Args...> t{std::forward<Args>(args)...};
        return {std::move(t)};
    }
    
#if SQLITE_VERSION_NUMBER >= 3007016
    
    template<class ...Args>
    core_functions::char_t_<Args...> char_(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
#endif
    
    template<class R, class ...Args>
    core_functions::coalesce_t<R, Args...> coalesce(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    template<class T>
    core_functions::trim_single_t<T> trim(T &&t) {
        return {std::move(t)};
    }
    
    template<class X, class Y>
    core_functions::trim_double_t<X, Y> trim(X &&x, Y &&y) {
        return {std::move(x), std::move(y)};
    }
    
    template<class X>
    core_functions::ltrim_single_t<X> ltrim(X &&x) {
        return {std::move(x)};
    }
    
    template<class X, class Y>
    core_functions::ltrim_double_t<X, Y> ltrim(X &&x, Y &&y) {
        return {std::move(x), std::move(y)};
    }
    
    template<class X>
    core_functions::rtrim_single_t<X> rtrim(X &&x) {
        return {std::move(x)};
    }
    
    template<class X, class Y>
    core_functions::rtrim_double_t<X, Y> rtrim(X &&x, Y &&y) {
        return {std::move(x), std::move(y)};
    }
    
    inline core_functions::changes_t changes() {
        return {};
    }
    
    template<class T>
    core_functions::length_t<T> length(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    template<class T>
    core_functions::abs_t<T> abs(T &&t) {
        std::tuple<T> args{std::forward<T>(t)};
        return {std::move(args)};
    }
    
    template<class T>
    core_functions::lower_t<T> lower(T &&t) {
        return {std::move(t)};
    }
    
    template<class T>
    core_functions::upper_t<T> upper(T t) {
        return {std::move(t)};
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
