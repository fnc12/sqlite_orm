#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../alias_traits.h"
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct content_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    constexpr bool is_content_v = polyfill::is_specialization_of_v<T, content_t>;

    template<class T>
    struct table_content_t {
        using mapped_type = T;
    };

    template<class T>
    constexpr bool is_table_content_v = polyfill::is_specialization_of_v<T, table_content_t>;
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
