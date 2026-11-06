#pragma once

/** @file
 *  Purpose: declare the concrete nodes that vocabulary files use directly without their definitions - in base-class
 *  overloads, partial specializations or metafunctions.
 *
 *  Intention: part of the vocabulary layer, whose rules `docs/internals/vocabulary-layer.md` holds; the definitions
 *  arrive through `node_definitions.h`.
 */

SQLITE_ORM_EXPORT namespace sqlite_orm { struct alias_tag; }

namespace sqlite_orm::internal {
    template<class... Cs>
    struct primary_key_t;

    template<class G, class S>
    struct column_field;

    template<class... Op>
    struct column_constraints;

    struct table_identifier;

    template<class T, class F>
    struct column_pointer;

    template<class O>
    struct table_reference;

    template<class C>
    struct indexed_column_t;

    struct order_by_base;
}
