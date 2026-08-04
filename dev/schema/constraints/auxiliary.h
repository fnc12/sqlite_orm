#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same, std::false_type
#endif

namespace sqlite_orm::internal {
#if SQLITE_VERSION_NUMBER >= 3024000
    /**
     *  Auxiliary virtual table column constraint
     */
    struct auxiliary_t {};

    template<class T>
    using is_auxiliary = std::is_same<T, auxiliary_t>;
#else
    template<class T>
    using is_auxiliary = std::false_type;
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
