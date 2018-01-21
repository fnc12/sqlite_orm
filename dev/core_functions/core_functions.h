//
//  core_functions.h
//  CPPTest
//
//  Created by John Zakharov on 20.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef core_functions_h
#define core_functions_h

#include <string>
#include <tuple>

namespace sqlite_orm {
    
    namespace core_functions {
        
        //  base class for operator overloading
        struct core_function_t {};
        
        /**
         *  LENGTH(x) function https://sqlite.org/lang_corefunc.html#length
         */
        template<class T>
        struct length_t : public core_function_t {
            T t;
            
            length_t(T t_):t(t_){}
            
            operator std::string() const {
                return "LENGTH";
            }
        };
        
        /**
         *  ABS(x) function https://sqlite.org/lang_corefunc.html#abs
         */
        template<class T>
        struct abs_t : public core_function_t {
            T t;
            
            abs_t(T t_):t(t_){}
            
            operator std::string() const {
                return "ABS";
            }
        };
        
        /**
         *  LOWER(x) function https://sqlite.org/lang_corefunc.html#lower
         */
        template<class T>
        struct lower_t : public core_function_t {
            T t;
            
            lower_t(T t_):t(t_){}
            
            operator std::string() const {
                return "LOWER";
            }
        };
        
        /**
         *  UPPER(x) function https://sqlite.org/lang_corefunc.html#upper
         */
        template<class T>
        struct upper_t : public core_function_t {
            T t;
            
            upper_t(T t_):t(t_){}
            
            operator std::string() const {
                return "UPPER";
            }
        };
        
        /**
         *  CHANGES() function https://sqlite.org/lang_corefunc.html#changes
         */
        struct changes_t : public core_function_t {
            
            operator std::string() const {
                return "CHANGES";
            }
        };
        
        /**
         *  TRIM(X) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class X>
        struct trim_single_t : public core_function_t {
            X x;
            
            trim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "TRIM";
            }
        };
        
        /**
         *  TRIM(X,Y) function https://sqlite.org/lang_corefunc.html#trim
         */
        template<class X, class Y>
        struct trim_double_t : public core_function_t {
            X x;
            Y y;
            
            trim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(trim_single_t<X>(0));
            }
        };
        
        /**
         *  LTRIM(X) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X>
        struct ltrim_single_t : public core_function_t {
            X x;
            
            ltrim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "LTRIM";
            }
        };
        
        /**
         *  LTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#ltrim
         */
        template<class X, class Y>
        struct ltrim_double_t : public core_function_t {
            X x;
            Y y;
            
            ltrim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(ltrim_single_t<X>(0));
            }
        };
        
        /**
         *  RTRIM(X) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X>
        struct rtrim_single_t : public core_function_t {
            X x;
            
            rtrim_single_t(X x_):x(x_){}
            
            operator std::string() const {
                return "RTRIM";
            }
        };
        
        /**
         *  RTRIM(X,Y) function https://sqlite.org/lang_corefunc.html#rtrim
         */
        template<class X, class Y>
        struct rtrim_double_t : public core_function_t {
            X x;
            Y y;
            
            rtrim_double_t(X x_, Y y_):x(x_), y(y_){}
            
            operator std::string() const {
                return static_cast<std::string>(rtrim_single_t<X>(0));
            }
        };
        
        
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        /**
         *  CHAR(X1,X2,...,XN) function https://sqlite.org/lang_corefunc.html#char
         */
        template<class ...Args>
        struct char_t_ : public core_function_t {
            using args_type = std::tuple<Args...>;
            
            args_type args;
            
            char_t_(args_type args_):args(args_){}
            
            operator std::string() const {
                return "CHAR";
            }
        };
        
        struct random_t : public core_function_t {
            
            operator std::string() const {
                return "RANDOM";
            }
        };
        
#endif
        template<class T, class ...Args>
        struct date_t : public core_function_t {
            using modifiers_type = std::tuple<Args...>;
            
            T timestring;
            modifiers_type modifiers;
            
            date_t(T timestring_, modifiers_type modifiers_):timestring(timestring_), modifiers(modifiers_){}
            
            operator std::string() const {
                return "DATE";
            }
        };
        
        template<class T, class ...Args>
        struct datetime_t : public core_function_t {
            using modifiers_type = std::tuple<Args...>;
            
            T timestring;
            modifiers_type modifiers;
            
            datetime_t(T timestring_, modifiers_type modifiers_):timestring(timestring_), modifiers(modifiers_){}
            
            operator std::string() const {
                return "DATETIME";
            }
        };
    }
}

#endif /* core_functions_h */
