//
//  column_result.h
//  CPPTest
//
//  Created by John Zakharov on 21.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef column_result_h
#define column_result_h

#include <sqlite3.h>
#include <memory>
#include <string>

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  This is a proxy class used to define what type must have result type depending on select
         *  arguments (member pointer, aggregate functions, etc). Below you can see specializations
         *  for different types. E.g. specialization for core_functions::length_t has `type` int cause
         *  LENGTH returns INTEGER in sqlite. Every column_result_t must have `type` type that equals
         *  c++ SELECT return type for T
         *  T - C++ type
         *  Ts - tables pack from storage. Rarely used. Required in asterisk to define columns mapped for a type
         */
        template<class T, class ...Ts>
        struct column_result_t;
        
        template<class O, class F, class ...Ts>
        struct column_result_t<F O::*, Ts...> {
            typedef F type;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<const F& (O::*)() const, Ts...> {
            typedef F type;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<void (O::*)(F), Ts...> {
            typedef F type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::length_t<T>, Ts...> {
            typedef int type;
        };
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        template<class ...Args, class ...Ts>
        struct column_result_t<core_functions::char_t_<Args...>, Ts...> {
            typedef std::string type;
        };
#endif
        
        template<class ...Ts>
        struct column_result_t<core_functions::random_t, Ts...> {
            typedef int type;
        };
        
        template<class ...Ts>
        struct column_result_t<core_functions::changes_t, Ts...> {
            typedef int type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::abs_t<T>, Ts...> {
            typedef std::shared_ptr<double> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::lower_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::upper_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::trim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::trim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::ltrim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::ltrim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::rtrim_single_t<X>, Ts...> {
            typedef std::string type;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::rtrim_double_t<X, Y>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::date_t<T, Args...>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::datetime_t<T, Args...>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::avg_t<T>, Ts...> {
            typedef double type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::count_t<T>, Ts...> {
            typedef int type;
        };
        
        template<class ...Ts>
        struct column_result_t<aggregate_functions::count_asterisk_t, Ts...> {
            typedef int type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::sum_t<T>, Ts...> {
            typedef std::shared_ptr<double> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::total_t<T>, Ts...> {
            typedef double type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_single_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_double_t<T>, Ts...> {
            typedef std::string type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::max_t<T>, Ts...> {
            typedef std::shared_ptr<typename column_result_t<T>::type> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::min_t<T>, Ts...> {
            typedef std::shared_ptr<typename column_result_t<T>::type> type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::distinct_t<T>, Ts...> {
            typedef typename column_result_t<T>::type type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::all_t<T>, Ts...> {
            typedef typename column_result_t<T>::type type;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::conc_t<L, R>, Ts...> {
            typedef std::string type;
        };
    }
}

#endif /* column_result_h */
