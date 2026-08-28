#pragma once

/** @file Projections yielding the mapped type a node stands for.
 *
 *  Unlike the projections reading a declared type name in `nested_types.h`, these destructure
 *  the node to the type it captured, so they name the concrete nodes they match. Whether the
 *  type they yield is actually mapped by a schema is not their question - that is answered one
 *  tier up, by the lookup algorithms in `schema/algorithms/table_lookup.h`.
 */

#include "../node_fwd.h"

namespace sqlite_orm::internal {
    /**
     *  Trait class used to define table mapped type by setter/getter/member
     *  T - member pointer
     *  `type` is a type which is mapped.
     *  E.g.
     *  -   `table_type_of<decltype(&User::id)>::type` is `User`
     *  -   `table_type_of<decltype(&User::getName)>::type` is `User`
     *  -   `table_type_of<decltype(&User::setName)>::type` is `User`
     *  -   `table_type_of<decltype(column<User>(&User::id))>::type` is `User`
     *  -   `table_type_of<decltype(derived->*&User::id)>::type` is `User`
     */
    template<class T>
    struct table_type_of;

    template<class O, class F>
    struct table_type_of<F O::*> {
        using type = O;
    };

    template<class T, class F>
    struct table_type_of<column_pointer<T, F>> {
        using type = T;
    };

    template<class C>
    struct table_type_of<indexed_column_t<C>> : table_type_of<C> {};

    template<class T>
    using table_type_of_t = typename table_type_of<T>::type;
}
