#pragma once

/** @file The references to the columns of a subselect, by which an outer statement refers to them.
 *
 *  Siblings to the expressions forming those columns (`column_expression.h`): a column formed by an aliased
 *  expression is referred to by its alias, a column of a mapped object by its member pointer, and any other
 *  expression by a numeric column alias.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <type_traits>  //  std::is_member_pointer_v
#include <tuple>  //  std::tuple, std::make_tuple, std::tuple_cat, std::get, std::tuple_size_v
#include <utility>  //  std::index_sequence, std::make_index_sequence
#endif
#endif

#include "../functional/type_traits.h"
#include "../vocabulary/node_traits.h"
#include "../ast/alias.h"
#include "../schema/algorithms/table_lookup.h"
#include "../schema/algorithms/column_lookup.h"

namespace sqlite_orm::internal {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    /**
     *  Return the column reference of a single result column expression, as a 1-tuple.
     *  `Idx` is the position of the result column, by which an expression without a name of its own is referenced.
     */
    template<size_t Idx, class E>
    auto get_column_reference(const E& col) {
        if constexpr (is_quoted_expression_v<E>) {
            return get_column_reference<Idx>(col._value);
        } else if constexpr (is_column_pointer_v<E>) {
            return get_column_reference<Idx>(col.field);
        } else if constexpr (std::is_member_pointer_v<E>) {
            // field/getter -> field/getter
            return std::make_tuple(col);
        } else if constexpr (is_as_node_v<E>) {
            // aliased expression -> alias_holder
            return std::tuple<alias_holder<alias_type_t<E>>>{};
        } else if constexpr (is_alias_holder_v<E>) {
            // colref -> alias_holder
            return std::tuple<E>{};
        } else {
            // any expression -> numeric column alias
            return std::tuple<alias_holder<decltype(n_to_colalias<Idx>())>>{};
        }
    }

    template<class Tpl, size_t... Idx>
    auto get_column_references(const Tpl& cols, std::index_sequence<Idx...>) {
        return std::tuple_cat(get_column_reference<Idx>(std::get<Idx>(cols))...);
    }

    /**
     *  Return a tuple of the column references of a select's result columns.
     */
    template<class DBOs, class Select, std::enable_if_t<is_select_v<Select>, bool> = true>
    auto get_column_references_of([[maybe_unused]] const DBOs& dbObjects, const Select& select) {
        using E = return_type_t<Select>;
        static_assert(!is_select_v<E> && !is_compound_operator_v<E>,
                      "A select statement cannot be a result column of a CTE's select statement");

        if constexpr (is_columns_v<E>) {
            return get_column_references(select.col.columns,
                                         std::make_index_sequence<std::tuple_size_v<columns_type_t<E>>>{});
        } else if constexpr (is_asterisk_v<E>) {
            // -> fields
            return get_table_column_fields(pick_table<type_t<E>>(dbObjects));
        } else {
            return get_column_reference<0>(select.col);
        }
    }
#endif
}
