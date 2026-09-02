#pragma once

/** @file Forward-declarations of concrete nodes that have to be used directly in template programming,
 *        e.g. for base-class overloads, partial specializations or metafunctions.
 */

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

    template<class C>
    struct indexed_column_t;

    struct order_by_base;

    template<class O>
    struct order_by_t;
}
