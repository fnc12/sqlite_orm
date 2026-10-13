#pragma once

/** @file Closed predicates for checking whether a node refers to a table or a record set,
 *        i.e. whether it may occupy the record set slot of a factory - FROM, JOIN, `get<>()`, `count<>()`, ...
 *
 *        They compose the structural table reference with the grammar alias traits; as concepts,
 *        with their C++17 fallback under the same guard.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::bool_constant
#endif

#include "../node_traits.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /** @short Specifies that a type refers to a mapped table (possibly aliased).
     */
    template<class T>
    concept orm_refers_to_table = (orm_table_reference<T> || orm_table_alias<T>);

    /** @short Specifies that a type refers to a recordset.
     */
    template<class T>
    concept orm_refers_to_recordset = (orm_table_reference<T> || orm_recordset_alias<T>);

    /** @short Specifies that a type is a mapped recordset (table reference).
     */
    template<class T>
    concept orm_mapped_recordset = (orm_table_reference<T> || orm_cte_moniker<T>);
#endif
}

namespace sqlite_orm::internal {
#ifndef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  Whether a node refers to a record set: a table reference or a record set alias.
     */
    template<class T>
    constexpr bool is_referring_to_recordset_v = is_table_reference_v<T> || is_recordset_alias_v<T>;

    template<class T>
    using is_referring_to_recordset = std::bool_constant<is_referring_to_recordset_v<T>>;
#endif
}
