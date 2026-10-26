#pragma once

/** @file Concepts classifying a single node referring to a record set or one of its columns:
 *        aliases, table references and table-valued function calls - the public face of
 *        the corresponding grammar and structural traits.
 *
 *        Concepts cannot be forward-declared, so this is a definition file within `traits/`.
 *        The alias concepts spell out their requirements rather than forwarding to the traits,
 *        so that a failed constraint tells which requirement is not met.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  //  std::derived_from, std::same_as
#include <type_traits>  //  std::remove_const_t
#endif
#endif

#include "../../functional/type_traits.h"  //  orm_names_type
#include "../node_fwd.h"  //  alias_tag
#include "grammar_traits_fwd.h"
#include "structural_traits_fwd.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class A>
    concept orm_alias = std::derived_from<A, alias_tag>;

    /** @short Specifies that a type is an alias of a column in a record set.
     *
     *  A column alias has the following traits:
     *  - is derived from `alias_tag`
     *  - must not have a nested `type` typename
     */
    template<class A>
    concept orm_column_alias = (orm_alias<A> && !orm_names_type<A>);

    /** @short Specifies that a type is an alias of any type of record set.
     *
     *  A record set alias has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a nested `type` typename, which refers to a mapped object.
     */
    template<class A>
    concept orm_recordset_alias = (orm_alias<A> && orm_names_type<A>);

    /** @short Specifies that a type is an alias of a concrete table.
     *
     *  A concrete table alias has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a `type` typename, which refers to another mapped object (i.e. doesn't refer to itself).
     */
    template<class A>
    concept orm_table_alias = (orm_recordset_alias<A> && !std::same_as<typename A::type, std::remove_const_t<A>>);

    /** @short Moniker of a CTE.
     *
     *  A CTE moniker has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a `type` typename, which refers to itself.
     */
    template<class A>
    concept orm_cte_moniker = (orm_recordset_alias<A> && std::same_as<typename A::type, std::remove_const_t<A>>);

    /** @short Specifies that a type is a reference of a concrete table, especially of a derived class.
     *
     *  A concrete table reference has the following traits:
     *  - specialization of `table_reference`, whose `type` typename references a mapped object.
     */
    template<class O>
    concept orm_table_reference = internal::is_table_reference_v<O>;

    /** @short Specifies that a type is a table-valued function call: `c<VTab>()(args...)`.
     */
    template<class T>
    concept orm_table_valued_expression = internal::is_table_valued_expression_v<T>;
#endif
}
