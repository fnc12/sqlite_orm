#pragma once

#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple
#include <utility>  //  std::pair
#include <functional>  //  std::reference_wrapper
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_filter.h"
#include "conditions.h"
#include "operators.h"
#include "select_constraints.h"
#include "prepared_statement.h"
#include "optional_container.h"
#include "core_functions.h"
#include "function.h"
#include "ast/excluded.h"
#include "ast/upsert_clause.h"
#include "ast/where.h"
#include "ast/into.h"
#include "ast/group_by.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class SFINAE = void>
        struct node_tuple {
            using type = std::tuple<T>;
        };

        template<class T>
        using node_tuple_t = typename node_tuple<T>::type;

        template<>
        struct node_tuple<void, void> {
            using type = std::tuple<>;
        };
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct node_tuple<as_optional_t<T>, void> : node_tuple<T> {};
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct node_tuple<std::reference_wrapper<T>, void> : node_tuple<T> {};

        template<class... Args>
        struct node_tuple<group_by_t<Args...>, void> : node_tuple<std::tuple<Args...>> {};

        template<class T, class... Args>
        struct node_tuple<group_by_with_having<T, Args...>, void> {
            using args_tuple = node_tuple_t<std::tuple<Args...>>;
            using expression_tuple = node_tuple_t<T>;
            using type = tuple_cat_t<args_tuple, expression_tuple>;
        };

        template<class T>
        struct node_tuple<T, match_if<is_upsert_clause, T>> : node_tuple<typename T::actions_tuple> {};

        template<class... Args>
        struct node_tuple<set_t<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T>
        struct node_tuple<excluded_t<T>, void> : node_tuple<T> {};

        template<class C>
        struct node_tuple<where_t<C>, void> : node_tuple<C> {};

        /**
         *  Column alias
         */
        template<class A>
        struct node_tuple<alias_holder<A>, void> : node_tuple<void> {};

        /**
         *  Column alias
         */
        template<char... C>
        struct node_tuple<column_alias<C...>, void> : node_tuple<void> {};

        /**
         *  Literal
         */
        template<class T>
        struct node_tuple<literal_holder<T>, void> : node_tuple<void> {};

        template<class E>
        struct node_tuple<order_by_t<E>, void> : node_tuple<E> {};

        template<class T>
        struct node_tuple<T, match_if<is_binary_condition, T>> {
            using node_type = T;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_node_tuple = node_tuple_t<left_type>;
            using right_node_tuple = node_tuple_t<right_type>;
            using type = tuple_cat_t<left_node_tuple, right_node_tuple>;
        };

        template<class L, class R, class... Ds>
        struct node_tuple<binary_operator<L, R, Ds...>, void> {
            using node_type = binary_operator<L, R, Ds...>;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_node_tuple = node_tuple_t<left_type>;
            using right_node_tuple = node_tuple_t<right_type>;
            using type = tuple_cat_t<left_node_tuple, right_node_tuple>;
        };

        template<class... Args>
        struct node_tuple<columns_t<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class L, class A>
        struct node_tuple<dynamic_in_t<L, A>, void> {
            using left_tuple = node_tuple_t<L>;
            using right_tuple = node_tuple_t<A>;
            using type = tuple_cat_t<left_tuple, right_tuple>;
        };

        template<class L, class... Args>
        struct node_tuple<in_t<L, Args...>, void> {
            using left_tuple = node_tuple_t<L>;
            using right_tuple = tuple_cat_t<node_tuple_t<Args>...>;
            using type = tuple_cat_t<left_tuple, right_tuple>;
        };

        template<class T>
        struct node_tuple<T, match_if<is_compound_operator, T>> {
            using node_type = T;
            using left_type = typename node_type::left_type;
            using right_type = typename node_type::right_type;
            using left_tuple = node_tuple_t<left_type>;
            using right_tuple = node_tuple_t<right_type>;
            using type = tuple_cat_t<left_tuple, right_tuple>;
        };

        template<class T, class... Args>
        struct node_tuple<select_t<T, Args...>, void> {
            using columns_tuple = node_tuple_t<T>;
            using args_tuple = tuple_cat_t<node_tuple_t<Args>...>;
            using type = tuple_cat_t<columns_tuple, args_tuple>;
        };

        template<class... Args>
        struct node_tuple<insert_raw_t<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class... Args>
        struct node_tuple<replace_raw_t<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T>
        struct node_tuple<into_t<T>, void> : node_tuple<void> {};

        template<class... Args>
        struct node_tuple<values_t<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class... Args>
        struct node_tuple<std::tuple<Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T, class R, class... Args>
        struct node_tuple<get_all_t<T, R, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T, class... Args>
        struct node_tuple<get_all_pointer_t<T, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct node_tuple<get_all_optional_t<T, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class... Args, class... Wargs>
        struct node_tuple<update_all_t<set_t<Args...>, Wargs...>, void> {
            using set_tuple = tuple_cat_t<node_tuple_t<Args>...>;
            using conditions_tuple = tuple_cat_t<node_tuple_t<Wargs>...>;
            using type = tuple_cat_t<set_tuple, conditions_tuple>;
        };

        template<class T, class... Args>
        struct node_tuple<remove_all_t<T, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T>
        struct node_tuple<having_t<T>, void> : node_tuple<T> {};

        template<class T, class E>
        struct node_tuple<cast_t<T, E>, void> : node_tuple<E> {};

        template<class T>
        struct node_tuple<exists_t<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<optional_container<T>, void> : node_tuple<T> {};

        template<class A, class T, class E>
        struct node_tuple<like_t<A, T, E>, void> {
            using arg_tuple = node_tuple_t<A>;
            using pattern_tuple = node_tuple_t<T>;
            using escape_tuple = node_tuple_t<E>;
            using type = tuple_cat_t<arg_tuple, pattern_tuple, escape_tuple>;
        };

        template<class A, class T>
        struct node_tuple<glob_t<A, T>, void> {
            using arg_tuple = node_tuple_t<A>;
            using pattern_tuple = node_tuple_t<T>;
            using type = tuple_cat_t<arg_tuple, pattern_tuple>;
        };

        template<class A, class T>
        struct node_tuple<between_t<A, T>, void> {
            using expression_tuple = node_tuple_t<A>;
            using lower_tuple = node_tuple_t<T>;
            using upper_tuple = node_tuple_t<T>;
            using type = tuple_cat_t<expression_tuple, lower_tuple, upper_tuple>;
        };

        template<class T>
        struct node_tuple<named_collate<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<is_null_t<T>, void> : node_tuple<T> {};

        template<class T>
        struct node_tuple<is_not_null_t<T>, void> : node_tuple<T> {};

        template<class C>
        struct node_tuple<negated_condition_t<C>, void> : node_tuple<C> {};

        template<class R, class S, class... Args>
        struct node_tuple<built_in_function_t<R, S, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class R, class S, class... Args>
        struct node_tuple<built_in_aggregate_function_t<R, S, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class F, class W>
        struct node_tuple<filtered_aggregate_function<F, W>, void> {
            using left_tuple = node_tuple_t<F>;
            using right_tuple = node_tuple_t<W>;
            using type = tuple_cat_t<left_tuple, right_tuple>;
        };

        template<class F, class... Args>
        struct node_tuple<function_call<F, Args...>, void> {
            using type = tuple_cat_t<node_tuple_t<Args>...>;
        };

        template<class T, class O>
        struct node_tuple<left_join_t<T, O>, void> : node_tuple<O> {};

        template<class T>
        struct node_tuple<on_t<T>, void> : node_tuple<T> {};

        // note: not strictly necessary as there's no binding support for USING;
        // we provide it nevertheless, in line with on_t.
        template<class T, class M>
        struct node_tuple<using_t<T, M>, void> : node_tuple<column_pointer<T, M>> {};

        template<class T, class O>
        struct node_tuple<join_t<T, O>, void> : node_tuple<O> {};

        template<class T, class O>
        struct node_tuple<left_outer_join_t<T, O>, void> : node_tuple<O> {};

        template<class T, class O>
        struct node_tuple<inner_join_t<T, O>, void> : node_tuple<O> {};

        template<class R, class T, class E, class... Args>
        struct node_tuple<simple_case_t<R, T, E, Args...>, void> {
            using case_tuple = node_tuple_t<T>;
            using args_tuple = tuple_cat_t<node_tuple_t<Args>...>;
            using else_tuple = node_tuple_t<E>;
            using type = tuple_cat_t<case_tuple, args_tuple, else_tuple>;
        };

        template<class L, class R>
        struct node_tuple<std::pair<L, R>, void> {
            using left_tuple = node_tuple_t<L>;
            using right_tuple = node_tuple_t<R>;
            using type = tuple_cat_t<left_tuple, right_tuple>;
        };

        template<class T, class E>
        struct node_tuple<as_t<T, E>, void> : node_tuple<E> {};

        template<class T>
        struct node_tuple<limit_t<T, false, false, void>, void> : node_tuple<T> {};

        template<class T, class O>
        struct node_tuple<limit_t<T, true, false, O>, void> {
            using type = tuple_cat_t<node_tuple_t<T>, node_tuple_t<O>>;
        };

        template<class T, class O>
        struct node_tuple<limit_t<T, true, true, O>, void> {
            using type = tuple_cat_t<node_tuple_t<O>, node_tuple_t<T>>;
        };
    }
}
