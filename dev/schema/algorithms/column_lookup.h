#pragma once

/** @file Lookup of a column definition within a tuple of database objects.
 *
 *  Schema-level algorithms like `table_lookup.h`: they locate the table definition mapping a column
 *  expression's object type - or the CTE a moniker names - and find the column within it.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple_size, std::tuple_element, std::get
#endif

#include "../../functional/index_sequence_util.h"
#include "../../functional/type_traits.h"
#include "../../tuple_helper/tuple_traits.h"
#include "../../tuple_helper/tuple_filter.h"
#include "../../tuple_helper/tuple_iteration.h"
#include "../../vocabulary/node_traits.h"
#include "../../vocabulary/node_algorithms.h"  //  col_index_sequence_of, col_index_sequence_with
#include "../../cte_types.h"
#include "../db_objects.h"
#include "table_lookup.h"

namespace sqlite_orm::internal {
    /**
     *  Find column name by its type and member pointer.
     */
    template<class Lookup, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
    const std::string* find_column_name(const DBOs& dbObjects, F Lookup::* field) {
        return pick_table<mapped_type_proxy_t<Lookup>>(dbObjects).find_column_name(field);
    }

    /**
     *  Materialize column pointer:
     *  1. by explicit object type and member pointer.
     *  2. by moniker and member pointer.
     *  3. by moniker and alias_holder<>.
     *
     *  internal note: `find_column_name()` looks up a column alias in a CTE directly, without going through
     *  `cte_table<>::find_column_name()`
     */
    template<class CP,
             class DBOs,
             std::enable_if_t<std::conjunction_v<is_db_objects<DBOs>, is_column_pointer<CP>>, bool> = true>
    constexpr decltype(auto) materialize_column_pointer(const DBOs&, const CP& cp) {
        if constexpr (is_alias_holder_v<field_type_t<CP>>) {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            using table_type = schema_pick_table_t<type_t<CP>, DBOs>;
            using cte_colrefs_tuple = typename cte_mapper_type_t<table_type>::final_colrefs_tuple;
            using cte_fields_type = typename cte_mapper_type_t<table_type>::fields_type;

            // lookup the column alias in the final column references
            using colalias_index = find_tuple_type<cte_colrefs_tuple, field_type_t<CP>>;
            static_assert(colalias_index::value < std::tuple_size_v<cte_colrefs_tuple>,
                          "No such column mapped into the CTE");

            return &aliased_field<type_t<field_type_t<CP>>,
                                  std::tuple_element_t<colalias_index::value, cte_fields_type>>::field;
#endif
        } else {
            return cp.field;
        }
    }

    /**
     *  Find column name by:
     *  1. by explicit object type and member pointer.
     *  2. by moniker and member pointer.
     *  3. by moniker and alias_holder<>.
     */
    template<class CP,
             class DBOs,
             std::enable_if_t<std::conjunction_v<is_db_objects<DBOs>, is_column_pointer<CP>>, bool> = true>
    const std::string* find_column_name(const DBOs& dbObjects, const CP& cp) {
        if constexpr (is_alias_holder_v<field_type_t<CP>>) {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            using table_type = schema_pick_table_t<type_t<CP>, DBOs>;
            using cte_colrefs_tuple = typename cte_mapper_type_t<table_type>::final_colrefs_tuple;
            using column_index_sequence = col_index_sequence_of<elements_type_t<table_type>>;

            // note: even though the columns contain the [`aliased_field<>::*`] we perform the lookup using the column references.
            // lookup the column alias in the final column references
            using colalias_index = find_tuple_type<cte_colrefs_tuple, field_type_t<CP>>;
            static_assert(colalias_index::value < std::tuple_size_v<cte_colrefs_tuple>,
                          "No such column mapped into the CTE");

            // note: we could "materialize" the alias to an `aliased_field<>::*` and use the regular `cte_table<>::find_column_name()` mechanism;
            //       however we have the column index already.
            // lookup column in base_table<>'s elements
            constexpr size_t ColIdx = index_sequence_value_at<colalias_index::value>(column_index_sequence{});
            auto& table = pick_table<type_t<CP>>(dbObjects);
            return &std::get<ColIdx>(table.elements).name;
#else
            return nullptr;
#endif
        } else {
            auto field = materialize_column_pointer(dbObjects, cp);
            return pick_table<type_t<CP>>(dbObjects).find_column_name(field);
        }
    }

    /**
     *  Checks whether the column with the specified name has a column-level `UNIQUE` constraint
     *  or is contained in a table-level `UNIQUE` constraint of the given table.
     */
    template<class Table, class DBOs, satisfies<is_db_objects, DBOs> = true>
    bool is_column_unique(const DBOs& dbObjects, const Table& table, const std::string& name) {
        using elements_type = elements_type_t<Table>;

        bool result = false;
        iterate_tuple(table.elements,
                      col_index_sequence_with<elements_type, is_unique>{},
                      [&result, &name](auto& column) {
                          if (column.name == name) {
                              result = true;
                          }
                      });
        if (!result) {
            iterate_tuple(
                table.elements,
                filter_tuple_sequence_t<elements_type, is_unique>{},
                [&dbObjects, &result, &name](auto& uniqueConstraint) {
                    iterate_tuple(uniqueConstraint.columns, [&dbObjects, &result, &name](auto& columnExpression) {
                        if (const std::string* columnName = find_column_name(dbObjects, columnExpression)) {
                            if (*columnName == name) {
                                result = true;
                            }
                        }
                    });
                });
        }
        return result;
    }
}
