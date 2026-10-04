#pragma once

/** @file Selection of the base tables or views within a tuple of database objects, and computations over them.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_pointer
#endif

#include "../../functional/type_traits.h"
#include "../../tuple_helper/tuple_filter.h"
#include "../../tuple_helper/tuple_iteration.h"
#include "../../vocabulary/node_traits.h"
#include "../db_objects.h"

namespace sqlite_orm::internal {
    /**
     *  The positions of the base tables in a tuple of database objects.
     */
    template<class DBOs>
    using tables_index_sequence = filter_tuple_sequence_t<DBOs, is_base_table>;

#ifdef SQLITE_ORM_WITH_VIEW
    /**
     *  The positions of the views in a tuple of database objects.
     */
    template<class DBOs>
    using views_index_sequence = filter_tuple_sequence_t<DBOs, is_view>;
#endif

    /**
     *  The number of foreign keys over all base tables of a tuple of database objects.
     */
    template<class DBOs, satisfies<is_db_objects, DBOs> = true>
    constexpr int foreign_keys_count() {
        int res = 0;
        iterate_tuple<DBOs>(tables_index_sequence<DBOs>{}, [&res](const auto* dummy) {
            using table_type = std::remove_pointer_t<decltype(dummy)>;
            res += table_type::template count_of<is_foreign_key>();
        });
        return res;
    }
}
