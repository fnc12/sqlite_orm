#pragma once

/** @file The tuple of database objects making up a schema.
 *
 *  A schema is represented as a tuple of database objects - tables, views, virtual tables,
 *  indexes and triggers. These are the generic facilities for recognizing and passing around
 *  such a tuple; locating an individual database object within one is the subject of
 *  `algorithms/table_lookup.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::true_type, std::false_type
#include <tuple>  //  std::tuple
#endif

#include "../functional/type_traits.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  //  is_with_clause

namespace sqlite_orm::internal {
    template<class... DBO>
    using db_objects_tuple = std::tuple<DBO...>;

    template<class T>
    struct is_db_objects : std::false_type {};

    template<class... DBO>
    struct is_db_objects<std::tuple<DBO...>> : std::true_type {};
    // note: cannot use `db_objects_tuple` alias template because older compilers have problems
    // to match `const db_objects_tuple`.
    template<class... DBO>
    struct is_db_objects<const std::tuple<DBO...>> : std::true_type {};

    /**
     *  Return passed in DBOs.
     *
     *  Note: A WITH clause prepends the database objects of its CTEs, see `cte_storage.h`.
     */
    template<class DBOs,
             class E,
             std::enable_if_t<std::conjunction_v<is_db_objects<DBOs>, std::negation<is_with_clause<E>>>, bool> = true>
    decltype(auto) db_objects_for_expression(DBOs& dbObjects, const E&) {
        return dbObjects;
    }
}
