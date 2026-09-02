#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple
#include <utility>  //  std::pair
#include <functional>  //  std::reference_wrapper
#endif

#include "type_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "conditions.h"
#include "operators.h"
#include "prepared_statement.h"
#include "optional_container.h"
#include "core_functions.h"
#include "function.h"
#include "ast/excluded.h"
#include "ast/upsert_clause.h"
#include "ast/into.h"
#include "ast/match.h"
#include "ast/cast.h"
#include "ast/in.h"
#include "ast/is_null.h"
#include "ast/is_not_null.h"
#include "window_functions.h"
#include "vocabulary/node_traits.h"

namespace sqlite_orm::internal {
    template<class T, class SFINAE = void>
    struct node_tuple {
        using type = std::tuple<T>;
    };

    template<class T>
    using node_tuple_t = typename node_tuple<T>::type;

    /*
     *  Node tuple for several types.
     */
    template<class... T>
    using node_tuple_for = conc_tuple<typename node_tuple<T>::type...>;

    template<>
    struct node_tuple<void, void> {
        using type = std::tuple<>;
    };

    template<class T>
    struct node_tuple<std::reference_wrapper<T>, void> : node_tuple<T> {};

    template<class... Args>
    struct node_tuple<std::tuple<Args...>, void> : node_tuple_for<Args...> {};

    template<class T>
    struct node_tuple<T, match_if<is_as_optional, T>> : node_tuple<expression_type_t<T>> {};

    /*
     *  The HAVING condition is only carried by the `group_by_with_having` spelling of the clause;
     *  for the plain one the detected expression type is `void`, contributing nothing.
     */
    template<class T>
    struct node_tuple<T, match_if<is_any_group_by, T>>
        : node_tuple_for<args_type_t<T>, polyfill::detected_or_t<void, expression_type_t, T>> {};

#if SQLITE_VERSION_NUMBER >= 3024000
    template<class Targets, class Actions>
    struct node_tuple<upsert_clause<Targets, Actions>, void> : node_tuple<Actions> {};
#endif

    template<class... Args>
    struct node_tuple<set_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class T, class X, class Y, class Z>
    struct node_tuple<highlight_t<T, X, Y, Z>, void> : node_tuple_for<X, Y, Z> {};

    template<class T>
    struct node_tuple<excluded_t<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_where<T>::value>> : node_tuple<expression_type_t<T>> {};

    template<class T, class X>
    struct node_tuple<match_with_table_t<T, X>, void> : node_tuple<X> {};

    template<class Field, class X>
    struct node_tuple<match_t<Field, X>, void> : node_tuple<X> {};

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

    template<class T>
    struct node_tuple<T, match_if<is_order_by, T>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_multi_order_by, T>> : node_tuple<args_type_t<T>> {};

    template<class L, class R>
    struct node_tuple<is_equal_with_table_t<L, R>, void> : node_tuple<R> {};

    template<class T>
    struct node_tuple<T, match_if<is_binary_condition, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_binary_operator, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_columns<T>::value>> : node_tuple<columns_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_struct<T>::value>> : node_tuple<columns_type_t<T>> {};

    template<class L, class A>
    struct node_tuple<dynamic_in_t<L, A>, void> : node_tuple_for<L, A> {};

    template<class L, class... Args>
    struct node_tuple<in_t<L, Args...>, void> : node_tuple_for<L, Args...> {};

    template<class T>
    struct node_tuple<T, match_if<is_compound_operator, T>> : node_tuple<expressions_tuple_t<T>> {};

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    template<class CTE>
    struct node_tuple<CTE, match_if<is_cte_binding, CTE>> : node_tuple<expression_type_t<CTE>> {};

    template<class With>
    struct node_tuple<With, match_if<is_with_clause, With>>
        : node_tuple_for<cte_type_t<With>, expression_type_t<With>> {};
#endif

    template<class T>
    struct node_tuple<T, match_if<is_select, T>> : node_tuple_for<return_type_t<T>, conditions_type_t<T>> {};

    template<class... Args>
    struct node_tuple<insert_raw_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<replace_raw_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class T>
    struct node_tuple<into_t<T>, void> : node_tuple<void> {};

    template<class... Args>
    struct node_tuple<values_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class T, class R, class... Args>
    struct node_tuple<get_all_t<T, R, Args...>, void> : node_tuple_for<Args...> {};

    template<class T, class... Args>
    struct node_tuple<get_all_pointer_t<T, Args...>, void> : node_tuple_for<Args...> {};

    template<class T, class... Args>
    struct node_tuple<get_all_optional_t<T, Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args, class... Wargs>
    struct node_tuple<update_all_t<set_t<Args...>, Wargs...>, void> : node_tuple_for<Args..., Wargs...> {};

    template<class T, class... Args>
    struct node_tuple<remove_all_t<T, Args...>, void> : node_tuple_for<Args...> {};

    template<class T, class E>
    struct node_tuple<cast_t<T, E>, void> : node_tuple<E> {};

    template<class T>
    struct node_tuple<exists_t<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<optional_container<T>, void> : node_tuple<T> {};

    template<class A, class T, class E>
    struct node_tuple<like_t<A, T, E>, void> : node_tuple_for<A, T, E> {};

    template<class A, class T>
    struct node_tuple<glob_t<A, T>, void> : node_tuple_for<A, T> {};

    template<class A, class T>
    struct node_tuple<between_t<A, T>, void> : node_tuple_for<A, T, T> {};

    template<class T>
    struct node_tuple<named_collate<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<is_null_t<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<is_not_null_t<T>, void> : node_tuple<T> {};

    template<class C>
    struct node_tuple<negated_condition_t<C>, void> : node_tuple<C> {};

    template<class T>
    struct node_tuple<unary_minus_t<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<bitwise_not_t<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<T, match_if<is_built_in_function, T>> : node_tuple<args_type_t<T>> {};

    template<class F, class W>
    struct node_tuple<filtered_aggregate_function<F, W>, void> : node_tuple_for<F, W> {};

    template<class F, class... Args>
    struct node_tuple<function_call<F, Args...>, void> : node_tuple_for<Args...> {};

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

    template<class T>
    struct node_tuple<T, match_if<is_case_expression, T>>
        : node_tuple_for<case_expression_type_t<T>, args_type_t<T>, else_expression_type_t<T>> {};

    template<class L, class R>
    struct node_tuple<std::pair<L, R>, void> : node_tuple_for<L, R> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_as_node<T>::value>> : node_tuple<expression_type_t<T>> {};

    /*
     *  The implicit `limit(offset, limit)` spelling binds its offset first; every other spelling binds
     *  the limit first, with a `void` offset expression contributing nothing.
     */
    template<class T>
    struct node_tuple<T, match_if<is_limit, T>>
        : mpl::conditional_t<T::offset_is_implicit_v,
                             node_tuple_for<offset_expression_type_t<T>, expression_type_t<T>>,
                             node_tuple_for<expression_type_t<T>, offset_expression_type_t<T>>> {};

    template<class T>
    struct node_tuple<T, match_if<is_from2, T>> : node_tuple<tuple_type_t<T>> {};

    /*
     *  Table reference as part of FROM clause: skip
     */
    template<class R>
    struct node_tuple<R, match_if<is_table_reference, R>> : node_tuple<void> {};

    template<class Table, class... Args>
    struct node_tuple<table_valued_expression<Table, Args...>, void> : node_tuple_for<Args...> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_preceding<T>::value>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_following<T>::value>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_unbounded_preceding<T>::value>> {
        using type = std::tuple<>;
    };

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_unbounded_following<T>::value>> {
        using type = std::tuple<>;
    };

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_current_row<T>::value>> {
        using type = std::tuple<>;
    };

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_frame_spec<T>::value>> : node_tuple_for<start_type_t<T>, end_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_partition_by<T>::value>> : node_tuple<args_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_window_ref<T>::value>> {
        using type = std::tuple<>;
    };

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_over<T>::value>> : node_tuple_for<function_type_t<T>, args_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_window_defn<T>::value>> : node_tuple<args_type_t<T>> {};

    template<>
    struct node_tuple<row_number_t, void> {
        using type = std::tuple<>;
    };

    template<>
    struct node_tuple<dense_rank_t, void> {
        using type = std::tuple<>;
    };

    template<>
    struct node_tuple<percent_rank_t, void> {
        using type = std::tuple<>;
    };

    template<>
    struct node_tuple<cume_dist_t, void> {
        using type = std::tuple<>;
    };

    template<class... Args>
    struct node_tuple<ntile_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<lag_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<lead_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<first_value_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<last_value_t<Args...>, void> : node_tuple_for<Args...> {};

    template<class... Args>
    struct node_tuple<nth_value_t<Args...>, void> : node_tuple_for<Args...> {};
}
