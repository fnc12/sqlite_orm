#pragma once

/** @file
 *  Purpose: represent the table an INSERT or REPLACE writes into.
 *
 *  Intention: a node header of `ast/crud/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary
 *  axes in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../vocabulary/node_traits.h"  //  orm_table_reference, auto_decay_table_ref_t
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct into_t {
        using type = T;
    };

    template<class T>
    constexpr bool is_into_v = polyfill::is_specialization_of<T, into_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::into_t<T> into() {
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_table_reference auto table>
    auto into() {
        return into<internal::auto_decay_table_ref_t<table>>();
    }
#endif
}
