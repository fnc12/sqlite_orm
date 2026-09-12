#pragma once

/** @file The DEFAULT VALUES modifier a raw INSERT or REPLACE takes in place of a row list.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct default_values_t {};

    template<class T>
    constexpr bool is_default_values_v = std::is_same<T, default_values_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Use this function to add `DEFAULT VALUES` modifier to raw `INSERT`.
     *
     *  @example
     *  ```
     *  storage.insert(into<Singer>(), default_values());
     *  ```
     */
    inline internal::default_values_t default_values() {
        return {};
    }
}
