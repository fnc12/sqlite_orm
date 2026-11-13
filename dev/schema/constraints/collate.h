#pragma once

/** @file
 *  Purpose: represent the COLLATE constraint of a column.
 *
 *  Intention: a node header of `schema/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../builtin/collations.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct collate_constraint_t {
        collate_argument argument = collate_argument::binary;
    };

    template<class T>
    constexpr bool is_collate_constraint_v = std::is_same<T, collate_constraint_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  https://www.sqlite.org/lang_createtable.html#collateclause
     */
    constexpr internal::collate_constraint_t collate_nocase() {
        return {internal::collate_argument::nocase};
    }

    /**
     *  https://www.sqlite.org/lang_createtable.html#collateclause
     */
    constexpr internal::collate_constraint_t collate_binary() {
        return {internal::collate_argument::binary};
    }

    /**
     *  https://www.sqlite.org/lang_createtable.html#collateclause
     */
    constexpr internal::collate_constraint_t collate_rtrim() {
        return {internal::collate_argument::rtrim};
    }
}
