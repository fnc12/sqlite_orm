#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple
#include <utility>  //  std::pair
#include <functional>  //  std::reference_wrapper
#endif

#include "functional/type_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "prepared_statement.h"
#include "optional_container.h"
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

    template<class T>
    struct node_tuple<T, match_if<is_upsert_clause, T>> : node_tuple<actions_tuple_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_set, T>> : node_tuple<assigns_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_excluded, T>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_where<T>::value>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<std::disjunction<is_match<T>, is_match_with_table<T>>::value>>
        : node_tuple<argument_type_t<T>> {};

    /**
     *  Column alias
     */
    template<class T>
    struct node_tuple<T, std::enable_if_t<std::disjunction<is_alias_holder<T>, is_column_alias<T>>::value>>
        : node_tuple<void> {};

    template<class T>
    struct node_tuple<T, match_if<is_order_by, T>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_multi_order_by, T>> : node_tuple<args_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_equal_with_table, T>> : node_tuple<right_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_binary_condition, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_binary_operator, T>> : node_tuple_for<left_type_t<T>, right_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_columns<T>::value>> : node_tuple<columns_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<is_struct<T>::value>> : node_tuple<columns_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_any_in, T>> : node_tuple_for<left_type_t<T>, argument_type_t<T>> {};

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

    template<class T>
    struct node_tuple<T, match_if<is_insert_raw, T>> : node_tuple<args_tuple_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_replace_raw, T>> : node_tuple<args_tuple_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_into, T>> : node_tuple<void> {};

    template<class T>
    struct node_tuple<T, match_if<is_values, T>> : node_tuple<args_tuple_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_any_get_all, T>> : node_tuple<conditions_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_update_all, T>> : node_tuple_for<set_type_t<T>, conditions_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_remove_all, T>> : node_tuple<conditions_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_cast, T>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_exists, T>> : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<optional_container<T>, void> : node_tuple<T> {};

    template<class T>
    struct node_tuple<T, match_if<is_like, T>>
        : node_tuple_for<expression_type_t<T>, pattern_type_t<T>, escape_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_glob, T>> : node_tuple_for<expression_type_t<T>, pattern_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_between, T>>
        : node_tuple_for<expression_type_t<T>, lower_type_t<T>, upper_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<std::disjunction<is_collate<T>, is_named_collate<T>>::value>>
        : node_tuple<expression_type_t<T>> {};

    template<class T>
    struct node_tuple<T, std::enable_if_t<std::disjunction<is_is_null<T>, is_is_not_null<T>>::value>>
        : node_tuple<argument_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_negated_condition, T>> : node_tuple<argument_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_unary_operator, T>> : node_tuple<argument_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_builtin_function_call, T>> : node_tuple<args_tuple_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_filtered_aggregate_function, T>>
        : node_tuple_for<function_type_t<T>, where_expression_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_app_function_call, T>> : node_tuple<args_tuple_t<T>> {};

    //  a join constrained by ON or USING; CROSS JOIN and NATURAL JOIN are leaves
    template<class T>
    struct node_tuple<T, std::enable_if_t<is_any_join_v<T> && polyfill::is_detected_v<on_type_t, T>>>
        : node_tuple<on_type_t<T>> {};

    template<class T>
    struct node_tuple<T, match_if<is_on, T>> : node_tuple<expression_type_t<T>> {};

    // note: not strictly necessary as there's no binding support for USING;
    // we provide it nevertheless, in line with on_t.
    template<class T>
    struct node_tuple<T, match_if<is_using, T>> : node_tuple<column_type_t<T>> {};

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

    template<class T>
    struct node_tuple<T, match_if<is_table_valued_expression, T>> : node_tuple<constraints_type_t<T>> {};

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
}
