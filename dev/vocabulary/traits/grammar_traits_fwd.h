#pragma once

/** @file Declarations of closed classifier/leaf traits + structural grouping traits.
 *        E.g. compound-statement, DML-statement, CTE-binding, ...
 *  
 *        Definitions of these traits are in the corresponding header files of the grammar nodes.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

// Schema classifier traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_column_v;

    template<class T>
    using is_column = std::bool_constant<is_column_v<T>>;

    template<class T>
    extern const bool is_hidden_column_v;

    template<class T>
    using is_hidden_column = std::bool_constant<is_hidden_column_v<T>>;

    template<class T>
    extern const bool is_base_table_v;

    template<class T>
    using is_base_table = std::bool_constant<is_base_table_v<T>>;

    template<class T>
    extern const bool is_view_v;

    template<class T>
    using is_view = std::bool_constant<is_view_v<T>>;

    template<class T>
    extern const bool is_virtual_table_v;

    template<class T>
    using is_virtual_table = std::bool_constant<is_virtual_table_v<T>>;

    template<class T>
    extern const bool is_index_v;

    template<class T>
    using is_index = std::bool_constant<is_index_v<T>>;

    template<class T>
    extern const bool is_trigger_v;

    template<class T>
    using is_trigger = std::bool_constant<is_trigger_v<T>>;

    template<class T>
    extern const bool is_primary_key_v;

    template<class T>
    using is_primary_key = std::bool_constant<is_primary_key_v<T>>;

    /**
     *  Nodes representing a PRIMARY KEY declared at a single column, i.e. carrying no column list of its own.
     *
     *  Note: there is deliberately no `is_table_primary_key` counterpart. A table-level PRIMARY KEY is
     *  `is_primary_key_v` and not `is_column_primary_key_v`, and the asymmetry is load-bearing: admitting a
     *  column PRIMARY KEY among the table elements is what lets `validate_base_table_definition()` diagnose it
     *  as an empty table primary key rather than as an unrecognized table element.
     */
    template<class T>
    extern const bool is_column_primary_key_v;

    template<class T>
    using is_column_primary_key = std::bool_constant<is_column_primary_key_v<T>>;

    /**
     *  Nodes carrying the AUTOINCREMENT keyword of a column primary key.
     *
     *  Note: AUTOINCREMENT only ever decorates a PRIMARY KEY, hence such a node is a primary key
     *  in its own right - `is_primary_key_v` and `is_column_primary_key_v` hold for it as well.
     */
    template<class T>
    extern const bool is_autoincrement_pk_v;

    template<class T>
    using is_autoincrement_pk = std::bool_constant<is_autoincrement_pk_v<T>>;

    template<class T>
    extern const bool is_foreign_key_v;

    template<class T>
    using is_foreign_key = std::bool_constant<is_foreign_key_v<T>>;

    template<class T>
    extern const bool is_generated_always_v;

    template<class T>
    using is_generated_always = std::bool_constant<is_generated_always_v<T>>;

    template<class T>
    extern const bool is_null_constraint_v;

    template<class T>
    using is_null_constraint = std::bool_constant<is_null_constraint_v<T>>;

    template<class T>
    extern const bool is_not_null_constraint_v;

    template<class T>
    using is_not_null_constraint = std::bool_constant<is_not_null_constraint_v<T>>;

    template<class T>
    extern const bool is_unique_v;

    template<class T>
    using is_unique = std::bool_constant<is_unique_v<T>>;

    template<class T>
    extern const bool is_default_v;

    template<class T>
    using is_default = std::bool_constant<is_default_v<T>>;

    template<class T>
    extern const bool is_check_v;

    template<class T>
    using is_check = std::bool_constant<is_check_v<T>>;

    template<class T>
    extern const bool is_collate_constraint_v;

    template<class T>
    using is_collate_constraint = std::bool_constant<is_collate_constraint_v<T>>;

    template<class T>
    extern const bool is_auxiliary_v;

    template<class T>
    using is_auxiliary = std::bool_constant<is_auxiliary_v<T>>;

    template<class T>
    extern const bool is_content_v;

    template<class T>
    using is_content = std::bool_constant<is_content_v<T>>;

    template<class T>
    extern const bool is_table_content_v;

    template<class T>
    using is_table_content = std::bool_constant<is_table_content_v<T>>;

    template<class T>
    extern const bool is_prefix_v;

    template<class T>
    using is_prefix = std::bool_constant<is_prefix_v<T>>;

    template<class T>
    extern const bool is_unindexed_v;

    template<class T>
    using is_unindexed = std::bool_constant<is_unindexed_v<T>>;

    template<class T>
    extern const bool is_tokenize_v;

    template<class T>
    using is_tokenize = std::bool_constant<is_tokenize_v<T>>;
}

// Classifier traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_select_v;

    template<class T>
    using is_select = std::bool_constant<is_select_v<T>>;

    /**
     *  Nodes binding a monikered subselect to a name, for the duration of a single statement: a CTE.
     */
    template<class T>
    extern const bool is_cte_binding_v;

    template<class T>
    using is_cte_binding = std::bool_constant<is_cte_binding_v<T>>;

    /**
     *  Nodes hinting at how a CTE's select statement is to be materialized: MATERIALIZED, NOT MATERIALIZED.
     */
    template<class T>
    extern const bool is_materialization_hint_v;

    template<class T>
    using is_materialization_hint = std::bool_constant<is_materialization_hint_v<T>>;

    /**
     *  Nodes representing a CASE expression, in both its simple and its searched form.
     */
    template<class T>
    extern const bool is_case_expression_v;

    template<class T>
    using is_case_expression = std::bool_constant<is_case_expression_v<T>>;

    template<class T>
    extern const bool is_with_clause_v;

    template<class T>
    using is_with_clause = std::bool_constant<is_with_clause_v<T>>;

    template<class T>
    extern const bool is_where_v;

    template<class T>
    using is_where = std::bool_constant<is_where_v<T>>;

    /**
     *  Nodes carrying a single ordering term, which is the argument a multi ORDER BY is composed of.
     */
    template<class T>
    extern const bool is_order_by_v;

    template<class T>
    using is_order_by = std::bool_constant<is_order_by_v<T>>;

    template<class T>
    extern const bool is_multi_order_by_v;

    template<class T>
    using is_multi_order_by = std::bool_constant<is_multi_order_by_v<T>>;

    /**
     *  Nodes carrying an ORDER BY assembled at runtime.
     */
    template<class T>
    extern const bool is_dynamic_order_by_v;

    template<class T>
    using is_dynamic_order_by = std::bool_constant<is_dynamic_order_by_v<T>>;

    //  the three above are DSL spellings of the one ORDER BY clause production,
    //  hence grouping them is what corresponds to the SQL grammar
    template<class T>
    extern const bool is_any_order_by_v;

    template<class T>
    using is_any_order_by = std::bool_constant<is_any_order_by_v<T>>;

    //  `group_by_t` and `group_by_with_having` are two DSL spellings of the one GROUP BY clause production,
    //  hence grouping them is what corresponds to the SQL grammar
    template<class T>
    extern const bool is_any_group_by_v;

    template<class T>
    using is_any_group_by = std::bool_constant<is_any_group_by_v<T>>;

    template<class T>
    extern const bool is_from_v;

    template<class T>
    using is_from = std::bool_constant<is_from_v<T>>;

    template<class T>
    extern const bool is_from2_v;

    template<class T>
    using is_from2 = std::bool_constant<is_from2_v<T>>;

    //  `from_t` and `from2_t` are two DSL spellings of the one FROM clause production,
    //  hence grouping them is what corresponds to the SQL grammar
    template<class T>
    extern const bool is_any_from_v;

    template<class T>
    using is_any_from = std::bool_constant<is_any_from_v<T>>;

    template<class T>
    extern const bool is_any_join_v;

    template<class T>
    using is_any_join = std::bool_constant<is_any_join_v<T>>;

    template<class T>
    extern const bool is_window_defn_v;

    template<class T>
    using is_window_defn = std::bool_constant<is_window_defn_v<T>>;

    template<class T>
    constexpr bool is_limit_v = false;

    template<class T>
    using is_limit = std::bool_constant<is_limit_v<T>>;

    template<class T>
    extern const bool is_offset_v;

    template<class T>
    using is_offset = std::bool_constant<is_offset_v<T>>;

    /**
     *  Nodes of a window function application and of the window definition it may carry:
     *  the OVER application itself, PARTITION BY, and the frame boundaries below.
     *
     *  The WINDOW clause of a select statement is `is_window_defn`, classified among the clauses above.
     */
    template<class T>
    extern const bool is_over_v;

    template<class T>
    using is_over = std::bool_constant<is_over_v<T>>;

    template<class T>
    extern const bool is_partition_by_v;

    template<class T>
    using is_partition_by = std::bool_constant<is_partition_by_v<T>>;

    /**
     *  Nodes referencing a window definition by name: OVER window-name.
     */
    template<class T>
    extern const bool is_window_ref_v;

    template<class T>
    using is_window_ref = std::bool_constant<is_window_ref_v<T>>;

    /**
     *  Nodes specifying a window frame: ROWS, RANGE or GROUPS BETWEEN a start and an end boundary.
     */
    template<class T>
    extern const bool is_frame_spec_v;

    template<class T>
    using is_frame_spec = std::bool_constant<is_frame_spec_v<T>>;

    /**
     *  Nodes representing a window frame boundary: UNBOUNDED PRECEDING, expr PRECEDING, CURRENT ROW,
     *  expr FOLLOWING, UNBOUNDED FOLLOWING.
     *
     *  Which end of a frame each of them may occupy is the subject of `is_frame_start_bound_v` and
     *  `is_frame_end_bound_v` in `algorithms/expression_element_predicates.h`.
     */
    template<class T>
    extern const bool is_unbounded_preceding_v;

    template<class T>
    using is_unbounded_preceding = std::bool_constant<is_unbounded_preceding_v<T>>;

    template<class T>
    extern const bool is_preceding_v;

    template<class T>
    using is_preceding = std::bool_constant<is_preceding_v<T>>;

    template<class T>
    extern const bool is_current_row_v;

    template<class T>
    using is_current_row = std::bool_constant<is_current_row_v<T>>;

    template<class T>
    extern const bool is_following_v;

    template<class T>
    using is_following = std::bool_constant<is_following_v<T>>;

    template<class T>
    extern const bool is_unbounded_following_v;

    template<class T>
    using is_unbounded_following = std::bool_constant<is_unbounded_following_v<T>>;

    template<class T>
    constexpr bool is_assign_v = false;

    template<class T>
    using is_assign = std::bool_constant<is_assign_v<T>>;

    template<class F, class SFINAE = void>
    constexpr bool is_scalar_udf_v = false;

    template<class F>
    using is_scalar_udf = std::bool_constant<is_scalar_udf_v<F>>;

    template<class F, class SFINAE = void>
    constexpr bool is_aggregate_udf_v = false;

    template<class F>
    using is_aggregate_udf = std::bool_constant<is_aggregate_udf_v<F>>;

    template<class T>
    extern const bool is_asterisk_v;

    template<class T>
    using is_asterisk = std::bool_constant<is_asterisk_v<T>>;

    template<class T>
    extern const bool is_as_node_v;

    template<class T>
    using is_as_node = std::bool_constant<is_as_node_v<T>>;

    /**
     *  Nodes representing the DISTINCT keyword.
     *
     *  Note: DISTINCT serves two roles - it is a rowset deduplicator in a simple select expression,
     *  and it deduplicates the argument rows of an aggregate function; hence it is classified on its own,
     *  next to the `is_rowset_deduplicator_v` grouping trait.
     */
    template<class T>
    extern const bool is_distinct_v;

    template<class T>
    using is_distinct = std::bool_constant<is_distinct_v<T>>;

    /**
     *  Nodes referencing a column of the row a trigger fires for: OLD.column, NEW.column.
     *
     *  Which of the two a trigger body may name depends on the trigger's event - OLD is absent from an
     *  INSERT trigger, NEW from a DELETE trigger -, which is why they are classified individually.
     */
    template<class T>
    extern const bool is_old_row_ref_v;

    template<class T>
    using is_old_row_ref = std::bool_constant<is_old_row_ref_v<T>>;

    template<class T>
    extern const bool is_new_row_ref_v;

    template<class T>
    using is_new_row_ref = std::bool_constant<is_new_row_ref_v<T>>;

    /**
     *  Nodes representing a RAISE expression of a trigger body:
     *  RAISE(IGNORE), RAISE(ROLLBACK | ABORT | FAIL, message).
     */
    template<class T>
    extern const bool is_raise_v;

    template<class T>
    using is_raise = std::bool_constant<is_raise_v<T>>;

    /**
     *  Nodes representing the keyword saying when a trigger fires: BEFORE, AFTER, INSTEAD OF.
     */
    template<class T>
    extern const bool is_trigger_timing_v;

    template<class T>
    using is_trigger_timing = std::bool_constant<is_trigger_timing_v<T>>;

    /**
     *  Nodes representing the keyword saying what makes a trigger fire: DELETE, INSERT, UPDATE.
     */
    template<class T>
    extern const bool is_trigger_event_v;

    template<class T>
    using is_trigger_event = std::bool_constant<is_trigger_event_v<T>>;

    /**
     *  Nodes pairing a trigger's timing with its event, which is one grammar production with an optional
     *  column list: BEFORE DELETE, AFTER UPDATE, AFTER UPDATE OF (columns).
     */
    template<class T>
    extern const bool is_trigger_event_spec_v;

    template<class T>
    using is_trigger_event_spec = std::bool_constant<is_trigger_event_spec_v<T>>;

    /**
     *  Nodes of the UPDATE OF form of that production, which alone carries a column list.
     *  A refinement of `is_trigger_event_spec_v`, which holds for these nodes as well.
     */
    template<class T>
    extern const bool is_trigger_update_of_v;

    template<class T>
    using is_trigger_update_of = std::bool_constant<is_trigger_update_of_v<T>>;

    /**
     *  Nodes specifying everything about a trigger but its name and body: the event spec above,
     *  the table it watches, FOR EACH ROW, and the WHEN condition.
     */
    template<class T>
    extern const bool is_trigger_spec_v;

    template<class T>
    using is_trigger_spec = std::bool_constant<is_trigger_spec_v<T>>;
}

// Role-based grammar traits
namespace sqlite_orm::internal {
    /**
     *  Nodes representing a keyword for a result set modifier (as part of a simple select expression): ALL, DISTINCT.
     */
    template<class T>
    extern const bool is_rowset_deduplicator_v;

    template<class T>
    using is_rowset_deduplicator = std::bool_constant<is_rowset_deduplicator_v<T>>;

    /**
     *  Nodes representing a compound operator (as part of a select expression): UNION, UNION ALL, INTERSECT, EXCEPT.
     */
    template<class T>
    extern const bool is_compound_operator_v;

    template<class T>
    using is_compound_operator = std::bool_constant<is_compound_operator_v<T>>;

    template<class T>
    extern const bool is_binary_operator_v;

    template<class T>
    using is_binary_operator = std::bool_constant<is_binary_operator_v<T>>;

    template<class T>
    extern const bool is_binary_condition_v;

    template<class T>
    using is_binary_condition = std::bool_constant<is_binary_condition_v<T>>;

    template<class T>
    extern const bool is_built_in_function_v;

    template<class T>
    using is_built_in_function = std::bool_constant<is_built_in_function_v<T>>;
}
