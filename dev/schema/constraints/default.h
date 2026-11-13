#pragma once

/** @file
 *  Purpose: represent the DEFAULT constraint.
 *
 *  Intention: a node header of `schema/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  DEFAULT constraint class.
     *  T is a value type.
     */
    template<class T>
    struct default_t {
        using value_type = T;

        value_type value;

        operator std::string() const {
            return "DEFAULT";
        }
    };

    template<class T>
    constexpr bool is_default_v = polyfill::is_specialization_of_v<T, default_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  https://www.sqlite.org/lang_createtable.html#dfltval
     */
    template<class T>
    constexpr internal::default_t<T> default_value(T t) {
        return {std::move(t)};
    }
}
