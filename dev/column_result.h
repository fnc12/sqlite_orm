#pragma once

#include "core_functions.h"
#include "aggregate_functions.h"
#include "select_constraints.h"
#include "operators.h"
#include "rowid.h"
#include "alias.h"

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
            using type = F;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<const F& (O::*)() const, Ts...> {
            using type = F;
        };
        
        template<class O, class F, class ...Ts>
        struct column_result_t<void (O::*)(F), Ts...> {
            using type = F;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::length_t<T>, Ts...> {
            using type = int;
        };
        
#if SQLITE_VERSION_NUMBER >= 3007016
        
        template<class ...Args, class ...Ts>
        struct column_result_t<core_functions::char_t_<Args...>, Ts...> {
            using type = std::string;
        };
#endif
        
        template<class ...Ts>
        struct column_result_t<core_functions::random_t, Ts...> {
            using type = int;
        };
        
        template<class ...Ts>
        struct column_result_t<core_functions::changes_t, Ts...> {
            using type = int;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::abs_t<T>, Ts...> {
            using type = std::shared_ptr<double>;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::lower_t<T>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<core_functions::upper_t<T>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::trim_single_t<X>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::trim_double_t<X, Y>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::ltrim_single_t<X>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::ltrim_double_t<X, Y>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class ...Ts>
        struct column_result_t<core_functions::rtrim_single_t<X>, Ts...> {
            using type = std::string;
        };
        
        template<class X, class Y, class ...Ts>
        struct column_result_t<core_functions::rtrim_double_t<X, Y>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::date_t<T, Args...>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Args, class ...Ts>
        struct column_result_t<core_functions::datetime_t<T, Args...>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::avg_t<T>, Ts...> {
            using type = double;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::count_t<T>, Ts...> {
            using type = int;
        };
        
        template<class ...Ts>
        struct column_result_t<aggregate_functions::count_asterisk_t, Ts...> {
            using type = int;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::sum_t<T>, Ts...> {
            using type = std::shared_ptr<double>;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::total_t<T>, Ts...> {
            using type = double;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_single_t<T>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::group_concat_double_t<T>, Ts...> {
            using type = std::string;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::max_t<T>, Ts...> {
            using type = std::shared_ptr<typename column_result_t<T>::type>;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<aggregate_functions::min_t<T>, Ts...> {
            using type = std::shared_ptr<typename column_result_t<T>::type>;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::distinct_t<T>, Ts...> {
            using type = typename column_result_t<T>::type;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::all_t<T>, Ts...> {
            using type = typename column_result_t<T>::type;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::conc_t<L, R>, Ts...> {
            using type = std::string;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::add_t<L, R>, Ts...> {
            using type = double;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::sub_t<L, R>, Ts...> {
            using type = double;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::mul_t<L, R>, Ts...> {
            using type = double;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::div_t<L, R>, Ts...> {
            using type = double;
        };
        
        template<class L, class R, class ...Ts>
        struct column_result_t<internal::mod_t<L, R>, Ts...> {
            using type = double;
        };
        
        template<class ...Ts>
        struct column_result_t<internal::rowid_t, Ts...> {
            using type = int64;
        };
        
        template<class ...Ts>
        struct column_result_t<internal::oid_t, Ts...> {
            using type = int64;
        };
        
        template<class ...Ts>
        struct column_result_t<internal::_rowid_t, Ts...> {
            using type = int64;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::table_rowid_t<T>, Ts...> {
            using type = int64;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::table_oid_t<T>, Ts...> {
            using type = int64;
        };
        
        template<class T, class ...Ts>
        struct column_result_t<internal::table__rowid_t<T>, Ts...> {
            using type = int64;
        };
        
        template<class T, class C, class ...Ts>
        struct column_result_t<internal::alias_column_t<T, C>, Ts...> {
            using type = typename column_result_t<C>::type;
        };
        
        template<class T, class F, class ...Ts>
        struct column_result_t<internal::column_pointer<T, F>, Ts...> : column_result_t<F, Ts...> {};
    }
}
