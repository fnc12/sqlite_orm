#pragma once

/** @file
 *  Purpose: represent the keywords SQL reads as literal values.
 *
 *  Intention: a node header of `ast/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct current_time_t {};

    template<class T>
    constexpr bool is_current_time_v = std::is_same<T, current_time_t>::value;

    struct current_date_t {};

    template<class T>
    constexpr bool is_current_date_v = std::is_same<T, current_date_t>::value;

    struct current_timestamp_t {};

    template<class T>
    constexpr bool is_current_timestamp_v = std::is_same<T, current_timestamp_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    inline internal::current_time_t current_time() {
        return {};
    }

    inline internal::current_date_t current_date() {
        return {};
    }

    inline internal::current_timestamp_t current_timestamp() {
        return {};
    }
}