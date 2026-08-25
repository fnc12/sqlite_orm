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
    using is_column = polyfill::bool_constant<is_column_v<T>>;

    template<class T>
    extern const bool is_hidden_column_v;

    template<class T>
    using is_hidden_column = polyfill::bool_constant<is_hidden_column_v<T>>;

    template<class T>
    extern const bool is_base_table_v;

    template<class T>
    using is_base_table = polyfill::bool_constant<is_base_table_v<T>>;

    template<class T>
    extern const bool is_view_v;

    template<class T>
    using is_view = polyfill::bool_constant<is_view_v<T>>;

    template<class T>
    extern const bool is_virtual_table_v;

    template<class T>
    using is_virtual_table = polyfill::bool_constant<is_virtual_table_v<T>>;

    template<class T>
    extern const bool is_primary_key_v;

    template<class T>
    using is_primary_key = polyfill::bool_constant<is_primary_key_v<T>>;

    template<class T>
    extern const bool is_column_primary_key_v;

    template<class T>
    using is_column_primary_key = polyfill::bool_constant<is_column_primary_key_v<T>>;

    template<class T>
    extern const bool is_foreign_key_v;

    template<class T>
    using is_foreign_key = polyfill::bool_constant<is_foreign_key_v<T>>;

    template<class T>
    extern const bool is_generated_always_v;

    template<class T>
    using is_generated_always = polyfill::bool_constant<is_generated_always_v<T>>;

    template<class T>
    extern const bool is_null_constraint_v;

    template<class T>
    using is_null_constraint = polyfill::bool_constant<is_null_constraint_v<T>>;

    template<class T>
    extern const bool is_not_null_constraint_v;

    template<class T>
    using is_not_null_constraint = polyfill::bool_constant<is_not_null_constraint_v<T>>;

    template<class T>
    extern const bool is_unique_v;

    template<class T>
    using is_unique = polyfill::bool_constant<is_unique_v<T>>;

    template<class T>
    extern const bool is_default_v;

    template<class T>
    using is_default = polyfill::bool_constant<is_default_v<T>>;

    template<class T>
    extern const bool is_check_v;

    template<class T>
    using is_check = polyfill::bool_constant<is_check_v<T>>;

    template<class T>
    extern const bool is_collate_constraint_v;

    template<class T>
    using is_collate_constraint = polyfill::bool_constant<is_collate_constraint_v<T>>;

    template<class T>
    extern const bool is_auxiliary_v;

    template<class T>
    using is_auxiliary = polyfill::bool_constant<is_auxiliary_v<T>>;

    template<class T>
    extern const bool is_content_v;

    template<class T>
    using is_content = polyfill::bool_constant<is_content_v<T>>;

    template<class T>
    extern const bool is_table_content_v;

    template<class T>
    using is_table_content = polyfill::bool_constant<is_table_content_v<T>>;

    template<class T>
    extern const bool is_prefix_v;

    template<class T>
    using is_prefix = polyfill::bool_constant<is_prefix_v<T>>;

    template<class T>
    extern const bool is_unindexed_v;

    template<class T>
    using is_unindexed = polyfill::bool_constant<is_unindexed_v<T>>;

    template<class T>
    extern const bool is_tokenize_v;

    template<class T>
    using is_tokenize = polyfill::bool_constant<is_tokenize_v<T>>;
}

// Classifier traits
namespace sqlite_orm::internal {
    template<class T>
    extern const bool is_select_v;

    template<class T>
    using is_select = polyfill::bool_constant<is_select_v<T>>;

    template<class T>
    extern const bool is_with_clause_v;

    template<class T>
    using is_with_clause = polyfill::bool_constant<is_with_clause_v<T>>;

    template<class T>
    extern const bool is_where_v;

    template<class T>
    using is_where = polyfill::bool_constant<is_where_v<T>>;

    template<class T>
    extern const bool is_order_by_v;

    template<class T>
    using is_order_by = polyfill::bool_constant<is_order_by_v<T>>;

    template<class T>
    extern const bool is_group_by_v;

    template<class T>
    using is_group_by = polyfill::bool_constant<is_group_by_v<T>>;

    template<class T>
    extern const bool is_from_v;

    template<class T>
    using is_from = polyfill::bool_constant<is_from_v<T>>;

    template<class T>
    extern const bool is_from2_v;

    template<class T>
    using is_from2 = polyfill::bool_constant<is_from2_v<T>>;

    //  `from_t` and `from2_t` are two DSL spellings of the one FROM clause production,
    //  hence grouping them is what corresponds to the SQL grammar
    template<class T>
    extern const bool is_any_from_v;

    template<class T>
    using is_any_from = polyfill::bool_constant<is_any_from_v<T>>;

    template<class T>
    extern const bool is_any_join_v;

    template<class T>
    using is_any_join = polyfill::bool_constant<is_any_join_v<T>>;

    template<class T>
    extern const bool is_window_defn_v;

    template<class T>
    using is_window_defn = polyfill::bool_constant<is_window_defn_v<T>>;

    template<class T>
    constexpr bool is_limit_v = false;

    template<class T>
    using is_limit = polyfill::bool_constant<is_limit_v<T>>;

    template<class T>
    extern const bool is_offset_v;

    template<class T>
    using is_offset = polyfill::bool_constant<is_offset_v<T>>;

    template<class T>
    constexpr bool is_assign_v = false;

    template<class T>
    using is_assign = polyfill::bool_constant<is_assign_v<T>>;

    template<class F, class SFINAE = void>
    constexpr bool is_scalar_udf_v = false;

    template<class F>
    using is_scalar_udf = polyfill::bool_constant<is_scalar_udf_v<F>>;

    template<class F, class SFINAE = void>
    constexpr bool is_aggregate_udf_v = false;

    template<class F>
    using is_aggregate_udf = polyfill::bool_constant<is_aggregate_udf_v<F>>;

    template<class T>
    extern const bool is_asterisk_v;

    template<class T>
    using is_asterisk = polyfill::bool_constant<is_asterisk_v<T>>;

    template<class T>
    extern const bool is_as_node_v;

    template<class T>
    using is_as_node = polyfill::bool_constant<is_as_node_v<T>>;
}

// Role-based grammar traits
namespace sqlite_orm::internal {
    /**
     *  Nodes representing a keyword for a result set modifier (as part of a simple select expression): ALL, DISTINCT.
     */
    template<class T>
    extern const bool is_rowset_deduplicator_v;

    template<class T>
    using is_rowset_deduplicator = polyfill::bool_constant<is_rowset_deduplicator_v<T>>;

    /**
     *  Nodes representing a compound operator (as part of a select expression): UNION, UNION ALL, INTERSECT, EXCEPT.
     */
    template<class T>
    extern const bool is_compound_operator_v;

    template<class T>
    using is_compound_operator = polyfill::bool_constant<is_compound_operator_v<T>>;

    template<class T>
    extern const bool is_binary_operator_v;

    template<class T>
    using is_binary_operator = polyfill::bool_constant<is_binary_operator_v<T>>;

    template<class T>
    extern const bool is_binary_condition_v;

    template<class T>
    using is_binary_condition = polyfill::bool_constant<is_binary_condition_v<T>>;

    template<class T>
    extern const bool is_built_in_function_v;

    template<class T>
    using is_built_in_function = polyfill::bool_constant<is_built_in_function_v<T>>;
}
