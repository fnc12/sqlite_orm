#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

namespace sqlite_orm::internal {
    template<class T>
    struct tokenize_t {
        using value_type = T;

        value_type value;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  tokenize='...'' table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#tokenizers
     */
    template<class T>
    constexpr internal::tokenize_t<T> tokenize(T value) {
        return {std::move(value)};
    }
#endif
}
