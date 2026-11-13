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
#include "../../functional/mpl/convenience.h"
#include "../../functional/type_traits.h"
#include "../../vocabulary/node_algorithms.h"  //  col_index_sequence_of, mapped_type_of_t
#include "../../vocabulary/node_traits.h"
#include "../cte.h"
#include "../db_objects.h"
#include "table_lookup.h"

namespace sqlite_orm::internal {
    /**
     *  Find column name by its type and member pointer.
     */
    template<class Lookup, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
    const std::string* find_column_name(const DBOs& dbObjects, F Lookup::* field) {
        return pick_table<mapped_type_of_t<Lookup>>(dbObjects).find_column_name(field);
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
            using moniker_type = type_t<CP>;
            using table_type = schema_pick_table_t<moniker_type, DBOs>;
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
            using moniker_type = type_t<CP>;
            using table_type = schema_pick_table_t<moniker_type, DBOs>;
            using cte_colrefs_tuple = typename cte_mapper_type_t<table_type>::final_colrefs_tuple;
            using column_index_sequence = col_index_sequence_of<elements_type_t<table_type>>;

            // note: even though the columns contain the [`aliased_field<>::*`] we perform the lookup using the column references.
            // lookup the column alias in the final column references
            using colalias_index = find_tuple_type<cte_colrefs_tuple, field_type_t<CP>>;
            static_assert(colalias_index::value < std::tuple_size_v<cte_colrefs_tuple>,
                          "No such column mapped into the CTE");

            // note: we could "materialize" the alias to an `aliased_field<>::*` and use the regular `cte_table<>::find_column_name()` mechanism;
            //       however we have the column index already.
            // lookup column in ordinary_table<>'s elements
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
}
