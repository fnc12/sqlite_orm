#pragma once

/** @file
 *  Purpose: represent the CHECK constraint.
 *
 *  Intention: a node header of `schema/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct check_t {
        using expression_type = T;

        expression_type expression;
    };

    template<class T>
    constexpr bool is_check_v = polyfill::is_specialization_of_v<T, check_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  https://www.sqlite.org/lang_createtable.html#ckconst
     */
    template<class T>
    constexpr internal::check_t<T> check(T t) {
        return {std::move(t)};
    }
}
