#pragma once

/** @file Closed predicates for checking the validity of the elements of a DDL statement.
 *
 *  These answer whether a node is an admissible member of a column or table definition -
 *  the classification questions specific to a single DDL node, as opposed to the
 *  type-and-order questions about statement clauses in `clause_predicates.h`.
 */

#include "../../functional/mpl.h"
#include "../node_traits.h"
#include "field_predicates_fwd.h"  // is_rowid_alias_capable

namespace sqlite_orm::internal {
    /**
     *  COLUMN PRIMARY KEY INSERTABLE traits.
     *
     *  A column primary key is considered implicitly insertable if:
     *  - it is an INTEGER PRIMARY KEY (and thus an alias for the "rowid" key),
     *  - or has a default value.
     *
     *  In terms of C++ types, this means that the field type must be capable of representing a 64-bit signed integer,
     *  or the column is declared with a DEFAULT constraint.
     *
     *  Implementation note: using a struct template in favor of an alias template so that the stack leading to a deprecation message is shorter.
     */
    template<class Column>
    struct is_pkcol_implicitly_insertable
        : mpl::invoke_t<mpl::disjunction<mpl::always<is_rowid_alias_capable<field_type_t<Column>>>,  //
                                         check_if_has<is_default>>,
                        constraints_type_t<Column>> {};

    template<class T>
    using is_column_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column_primary_key>,
                                                                check_if<is_null_constraint>,
                                                                check_if<is_not_null_constraint>,
                                                                check_if<is_unique>,
                                                                check_if<is_default>,
                                                                check_if<is_check>,
                                                                check_if<is_collate_constraint>,
                                                                check_if<is_generated_always>,
                                                                check_if<is_unindexed>,
                                                                check_if<is_auxiliary>>,
                                               T>;

    template<class T>
    using is_base_table_constraint = mpl::invoke_t<
        mpl::disjunction<check_if<is_primary_key>, check_if<is_foreign_key>, check_if<is_unique>, check_if<is_check>>,
        T>;

    template<class T>
    using is_base_table_element_or_constraint =
        mpl::invoke_t<mpl::disjunction<check_if<is_column>, check_if<is_base_table_constraint>>, T>;

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    template<class T>
    using is_fts5_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                                                               check_if<is_prefix>,
                                                                               check_if<is_tokenize>,
                                                                               check_if<is_content>,
                                                                               check_if<is_table_content>>,
                                                              T>;
#endif
}
