#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../alias_traits.h"

namespace sqlite_orm::internal {
    template<class T>
    struct content_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    struct table_content_t {
        using mapped_type = T;
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
    /**
     *  content='' table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#contentless_tables
     */
    template<class T>
    constexpr internal::content_t<T> content(T value) {
        return {std::move(value)};
    }

    /**
     *  content='table' table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#external_content_tables
     */
    template<class T>
    constexpr internal::table_content_t<T> content() {
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  content='table' table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#external_content_tables
     */
    template<orm_table_reference auto table>
    constexpr auto content() {
        return content<internal::auto_decay_table_ref_t<table>>();
    }
#endif
#endif
}
