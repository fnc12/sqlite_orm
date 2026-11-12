#pragma once

/** @file
 *  Purpose: represent the configuration options and column options of an FTS5 table.
 *
 *  Intention: a node header of `schema/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../vocabulary/node_traits.h"  //  orm_table_reference, auto_decay_table_ref_t
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

    template<class T>
    struct prefix_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    constexpr bool is_prefix_v = polyfill::is_specialization_of_v<T, prefix_t>;

    template<class T>
    struct tokenize_t {
        using value_type = T;

        value_type value;
    };

    template<class T>
    constexpr bool is_tokenize_v = polyfill::is_specialization_of<T, tokenize_t>::value;

    struct unindexed_t {};

    template<class T>
    constexpr bool is_unindexed_v = std::is_same<T, unindexed_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_FTS5_SUPPORTED
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

    /**
     *  prefix=N table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#prefix_indexes
     */
    template<class T>
    constexpr internal::prefix_t<T> prefix(T value) {
        return {std::move(value)};
    }

    /**
     *  tokenize='...'' table constraint factory function. Used in FTS virtual tables.
     *
     *  https://www.sqlite.org/fts5.html#tokenizers
     */
    template<class T>
    constexpr internal::tokenize_t<T> tokenize(T value) {
        return {std::move(value)};
    }

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
