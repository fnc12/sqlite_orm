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
    /*
     *  Implementation note: the primary template is defined, not merely declared, so that the trait
     *  stays probeable. `table_type_of<indexed_column_t<C>>` below derives from `table_type_of<C>`;
     *  for a `C` that has no mapping - an expression index element, say - an undeclared primary makes
     *  that an incomplete base, which is a hard error while instantiating the class rather than a
     *  substitution failure in the immediate context, and so defeats any detection of the trait.
     */
    template<class T>
    struct table_type_of {};

    /*
     *  Implementation note: the member pointer case is spelled out rather than forwarded to
     *  `member_object_type_t`, which computes the same thing one tier down. The obvious definition
     *  is shorter than the indirection would be, and keeps the specializations readable side by side.
     */
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
