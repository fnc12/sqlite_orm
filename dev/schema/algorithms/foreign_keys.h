#pragma once

/** @file
 *  Purpose: compute over the foreign keys of a schema.
 *
 *  Intention: a schema-level algorithm, placed by `docs/internals/vocabulary-layer.md` "Schema-level algorithms";
 *  consumes the vocabulary layer.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_pointer
#endif

#include "../../functional/tuple_helper/tuple_iteration.h"
#include "../../functional/type_traits.h"
#include "../../vocabulary/node_algorithms.h"
#include "../../vocabulary/node_traits.h"
#include "../db_objects.h"

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
}
