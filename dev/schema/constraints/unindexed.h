#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#endif

#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct unindexed_t {};

    template<class T>
    constexpr bool is_unindexed_v = std::is_same<T, unindexed_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  UNINDEXED column constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#the_unindexed_column_option
     */
    constexpr internal::unindexed_t unindexed() {
        return {};
    }
#endif
}
