#pragma once

#include <type_traits>  //  std::enable_if, std::is_same, std::decay, std::is_arithmetic, std::is_base_of
#include <functional>  //  std::reference_wrapper

#include "functional/cxx_universal.h"  //  ::nullptr_t
#include "functional/cxx_type_traits_polyfill.h"
#include "functional/mpl.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_fy.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "tuple_helper/same_or_void.h"
#include "type_traits.h"
#include "member_traits/member_traits.h"
#include "mapped_type_proxy.h"
#include "core_functions.h"
#include "select_constraints.h"
#include "operators.h"
#include "rowid.h"
#include "column_result_proxy.h"
#include "alias.h"
#include "cte_types.h"
#include "storage_traits.h"
#include "function.h"
#include "ast/special_keywords.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Obtains the result type of expressions that form the columns of a select statement.
         *  
         *  This is a proxy class used to define what type must have result type depending on select
         *  arguments (member pointer, aggregate functions, etc). Below you can see specializations
         *  for different types. E.g. specialization for internal::length_t has `type` int cause
         *  LENGTH returns INTEGER in sqlite. Every column_result_t must have `type` type that equals
         *  c++ SELECT return type for T
         *  DBOs - db_objects_tuple type
         *  T - C++ type
         *  SFINAE - sfinae argument
         */
        template<class DBOs, class T, class SFINAE = void>
        struct column_result_t;

        template<class DBOs, class T>
        using column_result_of_t = typename column_result_t<DBOs, T>::type;

        template<class DBOs, class Tpl>
        using column_result_for_tuple_t =
            transform_tuple_t<Tpl, mpl::bind_front_fn<column_result_of_t, DBOs>::template fn>;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class DBOs, class T>
        struct column_result_t<DBOs, as_optional_t<T>, void> {
            using type = std::optional<column_result_of_t<DBOs, T>>;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, std::optional<T>, void> {
            using type = std::optional<T>;
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class DBOs, class L, class A>
        struct column_result_t<DBOs, dynamic_in_t<L, A>, void> {
            using type = bool;
        };

        template<class DBOs, class L, class... Args>
        struct column_result_t<DBOs, in_t<L, Args...>, void> {
            using type = bool;
        };

        template<class DBOs>
        struct column_result_t<DBOs, current_time_t, void> {
            using type = std::string;
        };

        template<class DBOs>
        struct column_result_t<DBOs, current_date_t, void> {
            using type = std::string;
        };

        template<class DBOs>
        struct column_result_t<DBOs, current_timestamp_t, void> {
            using type = std::string;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, T, match_if<std::is_member_pointer, T>> : member_field_type<T> {};

        template<class DBOs, class R, class S, class... Args>
        struct column_result_t<DBOs, built_in_function_t<R, S, Args...>, void> {
            using type = R;
        };

        template<class DBOs, class R, class S, class... Args>
        struct column_result_t<DBOs, built_in_aggregate_function_t<R, S, Args...>, void> {
            using type = R;
        };

        template<class DBOs, class F, class... Args>
        struct column_result_t<DBOs, function_call<F, Args...>, void> {
            using type = typename callable_arguments<F>::return_type;
        };

        template<class DBOs, class X, class... Rest, class S>
        struct column_result_t<DBOs, built_in_function_t<unique_ptr_result_of<X>, S, X, Rest...>, void> {
            using type = std::unique_ptr<column_result_of_t<DBOs, X>>;
        };

        template<class DBOs, class X, class S>
        struct column_result_t<DBOs, built_in_aggregate_function_t<unique_ptr_result_of<X>, S, X>, void> {
            using type = std::unique_ptr<column_result_of_t<DBOs, X>>;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, count_asterisk_t<T>, void> {
            using type = int;
        };

        template<class DBOs>
        struct column_result_t<DBOs, nullptr_t, void> {
            using type = nullptr_t;
        };

        template<class DBOs>
        struct column_result_t<DBOs, count_asterisk_without_type, void> {
            using type = int;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, distinct_t<T>, void> : column_result_t<DBOs, T> {};

        template<class DBOs, class T>
        struct column_result_t<DBOs, all_t<T>, void> : column_result_t<DBOs, T> {};

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, conc_t<L, R>, void> {
            using type = std::string;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, add_t<L, R>, void> {
            using type = double;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, sub_t<L, R>, void> {
            using type = double;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, mul_t<L, R>, void> {
            using type = double;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, div_t<L, R>, void> {
            using type = double;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, mod_t<L, R>, void> {
            using type = double;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, bitwise_shift_left_t<L, R>, void> {
            using type = int;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, bitwise_shift_right_t<L, R>, void> {
            using type = int;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, bitwise_and_t<L, R>, void> {
            using type = int;
        };

        template<class DBOs, class L, class R>
        struct column_result_t<DBOs, bitwise_or_t<L, R>, void> {
            using type = int;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, bitwise_not_t<T>, void> {
            using type = int;
        };

        template<class DBOs>
        struct column_result_t<DBOs, rowid_t, void> {
            using type = int64;
        };

        template<class DBOs>
        struct column_result_t<DBOs, oid_t, void> {
            using type = int64;
        };

        template<class DBOs>
        struct column_result_t<DBOs, _rowid_t, void> {
            using type = int64;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, table_rowid_t<T>, void> {
            using type = int64;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, table_oid_t<T>, void> {
            using type = int64;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, table__rowid_t<T>, void> {
            using type = int64;
        };

        template<class DBOs, class T, class C>
        struct column_result_t<DBOs, alias_column_t<T, C>, void> : column_result_t<DBOs, C> {};

        template<class DBOs, class T, class F>
        struct column_result_t<DBOs, column_pointer<T, F>, void> : column_result_t<DBOs, F> {};

#ifdef SQLITE_ORM_WITH_CTE
        template<class DBOs, class Moniker, class ColAlias>
        struct column_result_t<DBOs, column_pointer<Moniker, alias_holder<ColAlias>>, void> {
            using table_type = storage_pick_table_t<Moniker, DBOs>;
            using cte_mapper_type = cte_mapper_type_t<table_type>;

            // lookup ColAlias in the final column references
            using colalias_index =
                find_tuple_type<typename cte_mapper_type::final_colrefs_tuple, alias_holder<ColAlias>>;
            SQLITE_ORM_STASSERT(colalias_index::value <
                                    std::tuple_size_v<typename cte_mapper_type::final_colrefs_tuple>,
                                "No such column mapped into the CTE.");
            using type = std::tuple_element_t<colalias_index::value, typename cte_mapper_type::fields_type>;
        };
#endif

        template<class DBOs, class... Args>
        struct column_result_t<DBOs, columns_t<Args...>, void>
            : conc_tuple<tuplify_t<column_result_of_t<DBOs, std::decay_t<Args>>>...> {};

        template<class DBOs, class T, class... Args>
        struct column_result_t<DBOs, struct_t<T, Args...>, void> {
            using type = structure<T, tuple_cat_t<tuplify_t<column_result_of_t<DBOs, std::decay_t<Args>>>...>>;
        };

        template<class DBOs, class T, class... Args>
        struct column_result_t<DBOs, select_t<T, Args...>> : column_result_t<DBOs, T> {};

        template<class DBOs, class T>
        struct column_result_t<DBOs, T, match_if<is_compound_operator, T>> {
            using type =
                polyfill::detected_t<common_type_of_t, column_result_for_tuple_t<DBOs, typename T::expressions_tuple>>;
            SQLITE_ORM_STASSERT(!std::is_same<type, polyfill::nonesuch>::value,
                                "Compound select statements must return a common type");
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, T, match_if<is_binary_condition, T>> {
            using type = typename T::result_type;
        };

        template<class DBOs, class T, class X, class Y, class Z>
        struct column_result_t<DBOs, highlight_t<T, X, Y, Z>, void> {
            using type = std::string;
        };

        /**
         *  Result for the most simple queries like `SELECT 1`
         */
        template<class DBOs, class T>
        struct column_result_t<DBOs, T, match_if<std::is_arithmetic, T>> {
            using type = T;
        };

        /**
         *  Result for the most simple queries like `SELECT 'ototo'`
         */
        template<class DBOs>
        struct column_result_t<DBOs, const char*, void> {
            using type = std::string;
        };

        template<class DBOs>
        struct column_result_t<DBOs, std::string, void> {
            using type = std::string;
        };

        template<class DBOs, class T, class E>
        struct column_result_t<DBOs, as_t<T, E>, void> : column_result_t<DBOs, std::decay_t<E>> {};

        template<class DBOs, class T>
        struct column_result_t<DBOs, asterisk_t<T>, void>
            : storage_traits::storage_mapped_columns<DBOs, mapped_type_proxy_t<T>> {};

        template<class DBOs, class T>
        struct column_result_t<DBOs, object_t<T>, void> {
            using type = table_reference<T>;
        };

        template<class DBOs, class T, class E>
        struct column_result_t<DBOs, cast_t<T, E>, void> {
            using type = T;
        };

        template<class DBOs, class R, class T, class E, class... Args>
        struct column_result_t<DBOs, simple_case_t<R, T, E, Args...>, void> {
            using type = R;
        };

        template<class DBOs, class A, class T, class E>
        struct column_result_t<DBOs, like_t<A, T, E>, void> {
            using type = bool;
        };

        template<class DBOs, class A, class T>
        struct column_result_t<DBOs, glob_t<A, T>, void> {
            using type = bool;
        };

        template<class DBOs, class C>
        struct column_result_t<DBOs, negated_condition_t<C>, void> {
            using type = bool;
        };

        template<class DBOs, class T>
        struct column_result_t<DBOs, std::reference_wrapper<T>, void> : column_result_t<DBOs, T> {};
    }
}
