#pragma once

/** @file
 *  Purpose: represent the constraint that makes a column of a virtual table auxiliary.
 *
 *  Intention: a node header of `schema/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
#if SQLITE_VERSION_NUMBER >= 3024000
    /**
     *  An auxiliary column of a virtual table, a feature a virtual table module may offer.
     */
    struct auxiliary_t {};

    template<class T>
    constexpr bool is_auxiliary_v = std::is_same<T, auxiliary_t>::value;
#else
    template<class T>
    constexpr bool is_auxiliary_v = false;
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_FTS5_SUPPORTED
#if SQLITE_VERSION_NUMBER >= 3024000
    /**
     *  Auxiliary virtual table column
     *
     *  https://www.sqlite.org/rtree.html#auxiliary_columns
     */
    constexpr internal::auxiliary_t auxiliary() {
        return {};
    }
#endif
#endif
}
