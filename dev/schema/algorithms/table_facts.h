#pragma once

/** @file
 *  Purpose: state what holds of the tables of a schema and their columns.
 *
 *  Intention: a schema-level algorithm, placed by `docs/internals/vocabulary-layer.md` "Schema-level algorithms";
 *  consumes the vocabulary layer.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple_size, std::get
#include <type_traits>  //  std::remove_pointer
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../functional/field_of.h"
#include "../../functional/member_traits.h"
#include "../../functional/mpl/convenience.h"
#include "../../functional/tuple_helper/tuple_filter.h"
#include "../../functional/tuple_helper/tuple_iteration.h"
#include "../../functional/type_traits.h"
#include "../../vocabulary/node_algorithms.h"
#include "../../vocabulary/node_fwd.h"  //  insertable_table_definition, column_field
#include "../../vocabulary/node_traits.h"
#include "../column_identifier.h"
#include "../db_objects.h"
#include "column_lookup.h"

namespace sqlite_orm::internal {
    /**
     *  The number of foreign keys over all ordinary tables of a tuple of database objects.
     */
    template<class DBOs, satisfies<is_db_objects, DBOs> = true>
    constexpr int foreign_keys_count() {
        int res = 0;
        iterate_tuple<DBOs>(ordinary_tables_index_sequence<DBOs>{}, [&res](const auto* dummy) {
            using table_type = std::remove_pointer_t<decltype(dummy)>;
            res += table_type::template count_of<is_foreign_key>();
        });
        return res;
    }

    template<class... Cs, class G, class S>
    bool table_primary_key_contains([[maybe_unused]] const insertable_table_definition<Cs...>& definition,
                                    [[maybe_unused]] const column_field<G, S>& column) {
        bool res = false;
        // note: though `visit_table_primary_key()` does no work if a column primary key exists, we try to save the compiler some work with this check up front
        if constexpr (/*bool hasNoColumnPK =*/!insertable_table_definition<Cs...>::template count_of_columns_with<
                      is_primary_key>()) {
            definition.visit_table_primary_key([&column, &res](auto& primaryKey) {
                using colrefs_tuple = decltype(primaryKey._columns);
                using same_type_index_sequence =
                    filter_tuple_sequence_t<colrefs_tuple,
                                            check_if_is_type<member_field_type_t<G>>::template fn,
                                            member_field_type_t>;
                iterate_tuple(primaryKey._columns, same_type_index_sequence{}, [&res, &column](auto& memberPointer) {
                    if (compare_fields(memberPointer, column.member_pointer) ||
                        compare_fields(memberPointer, column.setter)) {
                        res = true;
                    }
                });
            });
        }
        return res;
    }

    template<class... Cs, class G, class S>
    bool is_single_table_primary_key([[maybe_unused]] const insertable_table_definition<Cs...>& definition,
                                     [[maybe_unused]] const column_field<G, S>& column) {
        bool res = false;
        // note: though `visit_table_primary_key()` does no work if a column primary key exists, we try to save the compiler some work with this check up front
        if constexpr (/*bool hasNoColumnPK =*/!insertable_table_definition<Cs...>::template count_of_columns_with<
                      is_primary_key>()) {
            definition.visit_table_primary_key([&column, &res](auto& primaryKey) {
                // note: use `decltype(primaryKey)` instead of `decltype(primaryKey.columns)` otherwise msvc 141 chokes on the `if constexpr` below
                using colrefs_tuple = columns_tuple_t<polyfill::remove_cvref_t<decltype(primaryKey)>>;
                if constexpr (std::tuple_size<colrefs_tuple>::value == 1) {
                    auto& memberPointer = std::get<0>(primaryKey._columns);
                    if (compare_fields(memberPointer, column.member_pointer) ||
                        compare_fields(memberPointer, column.setter)) {
                        res = true;
                    }
                }
            });
        }
        return res;
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
                      [&result, &name](const column_identifier& column) {
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
