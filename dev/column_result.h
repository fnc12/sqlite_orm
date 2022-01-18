#pragma once

#include <type_traits>  //  std::enable_if, std::is_same, std::decay, std::is_arithmetic
#include <tuple>  //  std::tuple
#include <functional>  //  std::reference_wrapper

#include "cxx_polyfill.h"
#include "type_traits.h"
#include "core_functions.h"
#include "select_constraints.h"
#include "operators.h"
#include "rowid.h"
#include "alias.h"
#include "column.h"
#include "cte_types.h"
#include "storage_traits.h"
#include "function.h"
#include "tuple_helper/tuple_transformer.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  Obains the result type of expressions that form the columns of a select statement.
         *  
         *  This is a proxy class used to define what type must have result type depending on select
         *  arguments (member pointer, aggregate functions, etc). Below you can see specializations
         *  for different types. E.g. specialization for internal::length_t has `type` int cause
         *  LENGTH returns INTEGER in sqlite. Every column_result_t must have `type` type that equals
         *  c++ SELECT return type for T
         *  T - C++ type
         *  SFINAE - sfinae argument
         */
        template<class St, class T, class SFINAE = void>
        struct column_result_t;

        template<class St, class T>
        using column_result_of_t = typename column_result_t<St, T>::type;

        template<class St, class T, class SFINAE = void>
        struct column_expression_type;

        /**
         *  Obains the expressions that form the columns of a subselect statement.
         */
        template<class St, class T>
        using column_expression_of_t = typename column_expression_type<St, T>::type;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class St, class T>
        struct column_result_t<St, as_optional_t<T>, void> {
            using type = std::optional<typename column_result_t<St, T>::type>;
        };

#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class St, class O, class F>
        struct column_result_t<St,
                               F O::*,
                               std::enable_if_t<std::is_member_pointer<F O::*>::value &&
                                                !std::is_member_function_pointer<F O::*>::value>> {
            using type = F;
        };

        template<class St, class O, class F, F O::*m>
        struct column_result_t<St, ice_t<m>, void> {
            using type = F;
        };

        template<class St, class L, class A>
        struct column_result_t<St, dynamic_in_t<L, A>, void> {
            using type = bool;
        };

        template<class St, class L, class... Args>
        struct column_result_t<St, in_t<L, Args...>, void> {
            using type = bool;
        };

        /**
         *  Common case for all getter types. Getter types are defined in column.h file
         */
        template<class St, class T>
        struct column_result_t<St, T, match_if<is_getter, T>> {
            using type = typename getter_traits<T>::field_type;
        };

        /**
         *  Common case for all setter types. Setter types are defined in column.h file
         */
        template<class St, class T>
        struct column_result_t<St, T, match_if<is_setter, T>> {
            using type = typename setter_traits<T>::field_type;
        };

        template<class St, class R, class S, class... Args>
        struct column_result_t<St, built_in_function_t<R, S, Args...>, void> {
            using type = R;
        };

        template<class St, class F, class... Args>
        struct column_result_t<St, function_call<F, Args...>, void> {
            using type = typename callable_arguments<F>::return_type;
        };

        template<class St, class X, class S>
        struct column_result_t<St, built_in_function_t<unique_ptr_result_of<X>, S, X>, void> {
            using type = std::unique_ptr<typename column_result_t<St, X>::type>;
        };

        template<class St, class T>
        struct column_result_t<St, count_asterisk_t<T>, void> {
            using type = int;
        };

        template<class St>
        struct column_result_t<St, count_asterisk_without_type, void> {
            using type = int;
        };

        template<class St, class T>
        struct column_result_t<St, distinct_t<T>, void> {
            using type = typename column_result_t<St, T>::type;
        };

        template<class St, class T>
        struct column_result_t<St, all_t<T>, void> {
            using type = typename column_result_t<St, T>::type;
        };

        template<class St, class L, class R>
        struct column_result_t<St, conc_t<L, R>, void> {
            using type = std::string;
        };

        template<class St, class L, class R>
        struct column_result_t<St, add_t<L, R>, void> {
            using type = double;
        };

        template<class St, class L, class R>
        struct column_result_t<St, sub_t<L, R>, void> {
            using type = double;
        };

        template<class St, class L, class R>
        struct column_result_t<St, mul_t<L, R>, void> {
            using type = double;
        };

        template<class St, class L, class R>
        struct column_result_t<St, div_t<L, R>, void> {
            using type = double;
        };

        template<class St, class L, class R>
        struct column_result_t<St, mod_t<L, R>, void> {
            using type = double;
        };

        template<class St, class L, class R>
        struct column_result_t<St, bitwise_shift_left_t<L, R>, void> {
            using type = int;
        };

        template<class St, class L, class R>
        struct column_result_t<St, bitwise_shift_right_t<L, R>, void> {
            using type = int;
        };

        template<class St, class L, class R>
        struct column_result_t<St, bitwise_and_t<L, R>, void> {
            using type = int;
        };

        template<class St, class L, class R>
        struct column_result_t<St, bitwise_or_t<L, R>, void> {
            using type = int;
        };

        template<class St, class T>
        struct column_result_t<St, bitwise_not_t<T>, void> {
            using type = int;
        };

        template<class St>
        struct column_result_t<St, rowid_t, void> {
            using type = int64;
        };

        template<class St>
        struct column_result_t<St, oid_t, void> {
            using type = int64;
        };

        template<class St>
        struct column_result_t<St, _rowid_t, void> {
            using type = int64;
        };

        template<class St, class T>
        struct column_result_t<St, table_rowid_t<T>, void> {
            using type = int64;
        };

        template<class St, class T>
        struct column_result_t<St, table_oid_t<T>, void> {
            using type = int64;
        };

        template<class St, class T>
        struct column_result_t<St, table__rowid_t<T>, void> {
            using type = int64;
        };

        template<class St, class T, class C>
        struct column_result_t<St, alias_column_t<T, C>, void> : column_result_t<St, C> {};

        template<class St, class T, class F>
        struct column_result_t<St, column_pointer<T, F>, void> : column_result_t<St, F> {};

        template<class St, class Label, size_t I>
        struct column_result_t<St, column_pointer<Label, polyfill::index_constant<I>>, void>
            : column_result_t<St, cte_getter_t<storage_object_type_t<storage_pick_impl_t<St, Label>>, I>> {};

        template<class St, class Label, class O, class F, F O::*m>
        struct column_result_t<St, column_pointer<Label, ice_t<m>>, void> {
            using type = F;
        };

        template<class St, class... Args>
        struct column_result_t<St, columns_t<Args...>, void> {
            using type = fields_t<column_result_of_t<St, std::decay_t<Args>>...>;
        };

        template<class St, class T, class... Args>
        struct column_result_t<St, select_t<T, Args...>> : column_result_t<St, T> {};

        template<class St, class T>
        struct column_result_t<St, T, typename std::enable_if<is_base_of_template<T, compound_operator>::value>::type> {
            using left_type = typename T::left_type;
            using right_type = typename T::right_type;
            using left_result = column_result_of_t<St, left_type>;
            using right_result = column_result_of_t<St, right_type>;
            static_assert(std::is_same<left_result, right_result>::value,
                          "Compound subselect queries must return same types");
            using type = left_result;
        };

        template<class St, class T>
        struct column_result_t<St, T, typename std::enable_if<is_base_of_template<T, binary_condition>::value>::type> {
            using type = typename T::result_type;
        };

        /**
         *  Result for the most simple queries like `SELECT 1`
         */
        template<class St, class T>
        struct column_result_t<St, T, match_if<std::is_arithmetic, T>> {
            using type = T;
        };

        /**
         *  Result for the most simple queries like `SELECT 'ototo'`
         */
        template<class St>
        struct column_result_t<St, const char*, void> {
            using type = std::string;
        };

        template<class St>
        struct column_result_t<St, std::string, void> {
            using type = std::string;
        };

        template<class St, class T, class E>
        struct column_result_t<St, as_t<T, E>, void> : column_result_t<St, typename std::decay<E>::type> {};

        template<class St, class T>
        struct column_result_t<St, asterisk_t<T>, match_if_not<std::is_base_of, alias_tag, T>> {
            using type = typename storage_traits::storage_mapped_columns<St, T>::type;
        };

        template<class St, class A>
        struct column_result_t<St, asterisk_t<A>, match_if<std::is_base_of, alias_tag, A>> {
            using type = typename storage_traits::storage_mapped_columns<St, type_t<A>>::type;
        };

        template<class St, class T>
        struct column_result_t<St, object_t<T>, void> {
            using type = T;
        };

        template<class St, class T, class E>
        struct column_result_t<St, cast_t<T, E>, void> {
            using type = T;
        };

        template<class St, class R, class T, class E, class... Args>
        struct column_result_t<St, simple_case_t<R, T, E, Args...>, void> {
            using type = R;
        };

        template<class St, class A, class T, class E>
        struct column_result_t<St, like_t<A, T, E>, void> {
            using type = bool;
        };

        template<class St, class A, class T>
        struct column_result_t<St, glob_t<A, T>, void> {
            using type = bool;
        };

        template<class St, class C>
        struct column_result_t<St, negated_condition_t<C>, void> {
            using type = bool;
        };

        template<class St, class T>
        struct column_result_t<St, std::reference_wrapper<T>, void> : column_result_t<St, T> {};

        template<class St, class T, class SFINAE>
        struct column_expression_type {
            using type = T;
        };

        template<class St, class T>
        struct column_expression_type<St, std::reference_wrapper<T>, void> : column_expression_type<St, T> {};

        // No CTE for object expressions.
        template<class St, class T>
        struct column_expression_type<St, object_t<T>, void> {
            static_assert(polyfill::always_false_v<T>, "Selecting an object in a subselect is not allowed.");
        };

        template<class St, class T>
        struct column_expression_type<St, asterisk_t<T>, match_if_not<std::is_base_of, alias_tag, T>>
            : storage_traits::storage_mapped_column_expressions<St, T> {};

        template<class A>
        struct add_column_alias {
            template<typename ColExpr>
            struct apply {
                using type = alias_column_t<A, ColExpr>;
            };
        };
        template<class St, class A>
        struct column_expression_type<St, asterisk_t<A>, match_if<std::is_base_of, alias_tag, A>>
            : tuple_transformer<typename storage_traits::storage_mapped_column_expressions<St, type_t<A>>::type,
                                add_column_alias<A>::template apply> {};

        template<class St, class O, class F, F O::*m>
        struct column_expression_type<St, ice_t<m>, void> : ice_t<m> {};

        template<class St, class... Args>
        struct column_expression_type<St, columns_t<Args...>, void> {
            using type = std::tuple<column_expression_of_t<St, std::decay_t<Args>>...>;
        };

        template<class St, class T, class... Args>
        struct column_expression_type<St, select_t<T, Args...>> : column_expression_type<St, T> {};
    }
}
