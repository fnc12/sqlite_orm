#pragma once

/** @file Extraction of composite column results from a result row, element by element: tuples, ad-hoc structures
 *        and mapped objects.
 */

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple
#include <utility>  //  std::index_sequence_for, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/gsl.h"
#include "../functional/type_traits.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../vocabulary/node_traits.h"  //  table_reference, is_table_reference_v
#include "../schema/algorithms/table_lookup.h"
#include "column_result_proxy.h"
#include "row_extractor.h"
#include "object_from_column_builder.h"

namespace sqlite_orm::internal {

    /*
     *  Helper to extract a structure from a rowset.
     */
    template<class R, class DBOs>
    struct struct_extractor;

    /*  
     *  Returns a value-based row extractor for an unmapped type,
     *  returns a structure extractor for a table reference, tuple or named struct.
     */
    template<class R, class DBOs>
    auto make_row_extractor([[maybe_unused]] const DBOs& dbObjects) {
        if constexpr (polyfill::is_specialization_of_v<R, std::tuple> ||
                      polyfill::is_specialization_of_v<R, structure> || is_table_reference_v<R>) {
            return struct_extractor<R, DBOs>{dbObjects};
        } else {
            return row_value_extractor<R>();
        }
    }

    /**
     *  Specialization for a tuple of top-level column results.
     */
    template<class DBOs, class... Args>
    struct struct_extractor<std::tuple<Args...>, DBOs> {
        const DBOs& db_objects;

        std::tuple<Args...> extract(orm_gsl::czstring columnText) const = delete;

        // note: expects to be called only from the top level, and therefore discards the index
        std::tuple<column_result_proxy_t<Args>...> extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
            int columnIndex = -1;
            return {make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
        }

        // unused to date
        std::tuple<column_result_proxy_t<Args>...> extract(sqlite3_stmt* stmt, int& columnIndex) const = delete;

        std::tuple<Args...> extract(sqlite3_value* value) const = delete;
    };

    /**
     *  Specialization for an unmapped structure to be constructed ad-hoc from column results.
     *  
     *  This plays together with `column_result_of_t`, which returns `struct_t<O>` as `structure<O>`
     */
    template<class O, class... Args, class DBOs>
    struct struct_extractor<structure<O, std::tuple<Args...>>, DBOs> {
        const DBOs& db_objects;

        O extract(orm_gsl::czstring columnText) const = delete;

        // note: expects to be called only from the top level, and therefore discards the index;
        // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
        //       see unit test tests/prepared_statement_tests/select.cpp/TEST_CASE("Prepared select")/SECTION("non-aggregate struct")
        template<class Ox = O, satisfies<is_eval_order_garanteed, Ox> = true>
        O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
            int columnIndex = -1;
            return O{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
        }

        template<class Ox = O, satisfies_not<is_eval_order_garanteed, Ox> = true>
        O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
            int columnIndex = -1;
            // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
            std::tuple<Args...> t{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
            return create_from_tuple<O>(std::move(t), std::index_sequence_for<Args...>{});
        }

        // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
        //       see unit test tests/prepared_statement_tests/select.cpp/TEST_CASE("Prepared select")/SECTION("non-aggregate struct")
        template<class Ox = O, satisfies<is_eval_order_garanteed, Ox> = true>
        O extract(sqlite3_stmt* stmt, int& columnIndex) const {
            --columnIndex;
            return O{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
        }

        template<class Ox = O, satisfies_not<is_eval_order_garanteed, Ox> = true>
        O extract(sqlite3_stmt* stmt, int& columnIndex) const {
            --columnIndex;
            // note: brace-init-list initialization guarantees order of evaluation, but only for aggregates and variadic constructors it seems.
            std::tuple<Args...> t{make_row_extractor<Args>(this->db_objects).extract(stmt, ++columnIndex)...};
            return create_from_tuple<O>(std::move(t), std::index_sequence_for<Args...>{});
        }

        O extract(sqlite3_value* value) const = delete;
    };

    /**
     *  Specialization for a table reference.
     *  
     *  This plays together with `column_result_of_t`, which returns `object_t<O>` as `table_reference<O>`
     */
    template<class O, class DBOs>
    struct struct_extractor<table_reference<O>, DBOs> {
        const DBOs& db_objects;

        O extract(orm_gsl::czstring columnText) const = delete;

        // note: expects to be called only from the top level, and therefore discards the index
        O extract(sqlite3_stmt* stmt, int&& /*nextColumnIndex*/ = 0) const {
            int columnIndex = 0;
            return this->extract(stmt, columnIndex);
        }

        O extract(sqlite3_stmt* stmt, int& columnIndex) const {
            O obj;
            object_from_column_builder<O> builder{obj, stmt, columnIndex};
            auto& table = pick_table<O>(this->db_objects);
            table.for_each_column(builder);
            columnIndex = builder.columnIndex;
            return obj;
        }

        O extract(sqlite3_value* value) const = delete;
    };
}
