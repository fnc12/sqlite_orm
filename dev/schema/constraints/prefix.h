#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct prefix_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    constexpr bool is_prefix_v = polyfill::is_specialization_of_v<T, prefix_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  prefix=N table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#prefix_indexes
     */
    template<class T>
    constexpr internal::prefix_t<T> prefix(T value) {
        return {std::move(value)};
    }
#endif
}
