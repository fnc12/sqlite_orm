#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct null_t {};

    template<class T>
    constexpr bool is_null_constraint_v = std::is_same<T, null_t>::value;

    struct not_null_t {};

    template<class T>
    constexpr bool is_not_null_constraint_v = std::is_same<T, not_null_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    constexpr internal::null_t null() {
        return {};
    }

    constexpr internal::not_null_t not_null() {
        return {};
    }
}
