#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
#if SQLITE_VERSION_NUMBER >= 3024000
    /**
     *  Auxiliary virtual table column constraint
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
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
#if SQLITE_VERSION_NUMBER >= 3024000
    /**
     *  Auxiliary virtual table column
     */
    constexpr internal::auxiliary_t auxiliary() {
        return {};
    }
#endif
#endif
}
