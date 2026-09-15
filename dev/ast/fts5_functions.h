#pragma once

/** @file The FTS5 auxiliary functions `highlight()`, `bm25()` and `snippet()`.
 *
 *  An FTS5 auxiliary function is an ordinary built-in function whose first argument is the FTS5 table's
 *  hidden column named like the table (`fts5::hidden::any`), which is how SQLite refers to the table
 *  in `bm25(posts)`: `posts` is a column reference. The factories accept that column alone as the first
 *  argument, and the general built-in function machinery does the rest - the column serializes like any
 *  other column and names its table for the FROM clause.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::make_tuple
#include <utility>  //  std::move
#include <type_traits>  //  std::enable_if
#endif

#include <sqlite3.h>

#include "../alias_traits.h"  //  is_recordset_alias_v
#include "../column_pointer.h"  //  column
#include "../vocabulary/node_algorithms.h"  //  hidden_column_of_vtab, hidden_field_of_vtab
#include "builtin_function.h"

namespace sqlite_orm::internal {
    struct highlight_string {
        std::string_view serialize() const {
            return "HIGHLIGHT";
        }
    };

    struct bm25_string {
        std::string_view serialize() const {
            return "BM25";
        }
    };

    struct snippet_string {
        std::string_view serialize() const {
            return "SNIPPET";
        }
    };
}

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
SQLITE_ORM_EXPORT namespace sqlite_orm {
    struct fts5;

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class CP, class X, class Y, class Z>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::builtin_function_t<std::string, internal::highlight_string, CP, X, Y, Z>
    highlight(CP theAnyField, X x, Y y, Z z) {
        return {std::make_tuple(std::move(theAnyField), std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class Hidden, class F, class X, class Y, class Z>
        requires (internal::hidden_field_of_vtab<Hidden, F, fts5>)
    constexpr internal::builtin_function_t<std::string, internal::highlight_string, F Hidden::*, X, Y, Z>
    highlight(F Hidden::* theAnyField, X x, Y y, Z z) {
        return {std::make_tuple(theAnyField, std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class CP, class... Ws>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::builtin_function_t<double, internal::bm25_string, CP, Ws...> bm25(CP theAnyField,
                                                                                          Ws... weights) {
        return {std::make_tuple(std::move(theAnyField), std::move(weights)...)};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class Hidden, class F, class... Ws>
        requires (internal::hidden_field_of_vtab<Hidden, F, fts5>)
    constexpr internal::builtin_function_t<double, internal::bm25_string, F Hidden::*, Ws...>
    bm25(F Hidden::* theAnyField, Ws... weights) {
        return {std::make_tuple(theAnyField, std::move(weights)...)};
    }

    /**
     *  The FTS5 snippet auxiliary function: returns the text of the column `columnIndex` with the matches
     *  surrounded by `matchOpen`/`matchClose`, truncated to at most `tokenCount` tokens,
     *  with `ellipses` marking the truncations.
     *  See https://www.sqlite.org/fts5.html#the_snippet_function
     */
    template<class CP, class X1, class X2, class X3, class X4, class X5>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::builtin_function_t<std::string, internal::snippet_string, CP, X1, X2, X3, X4, X5>
    snippet(CP theAnyField, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(theAnyField),
                                std::move(columnIndex),
                                std::move(matchOpen),
                                std::move(matchClose),
                                std::move(ellipses),
                                std::move(tokenCount))};
    }

    /**
     *  The FTS5 snippet auxiliary function: returns the text of the column `columnIndex` with the matches
     *  surrounded by `matchOpen`/`matchClose`, truncated to at most `tokenCount` tokens,
     *  with `ellipses` marking the truncations.
     *  See https://www.sqlite.org/fts5.html#the_snippet_function
     */
    template<class Hidden, class F, class X1, class X2, class X3, class X4, class X5>
        requires (internal::hidden_field_of_vtab<Hidden, F, fts5>)
    constexpr internal::builtin_function_t<std::string, internal::snippet_string, F Hidden::*, X1, X2, X3, X4, X5>
    snippet(F Hidden::* theAnyField, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(theAnyField,
                                std::move(columnIndex),
                                std::move(matchOpen),
                                std::move(matchClose),
                                std::move(ellipses),
                                std::move(tokenCount))};
    }
#else
    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class CP,
             class X,
             class Y,
             class Z,
             std::enable_if_t<internal::is_hidden_column_of_vtab_v<CP, fts5>, bool> = true>
    constexpr internal::builtin_function_t<std::string, internal::highlight_string, CP, X, Y, Z>
    highlight(CP theAnyField, X x, Y y, Z z) {
        return {std::make_tuple(std::move(theAnyField), std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class Hidden,
             class F,
             class X,
             class Y,
             class Z,
             std::enable_if_t<internal::is_hidden_field_of_vtab_v<Hidden, F, fts5>, bool> = true>
    constexpr internal::builtin_function_t<std::string, internal::highlight_string, F Hidden::*, X, Y, Z>
    highlight(F Hidden::* theAnyField, X x, Y y, Z z) {
        return {std::make_tuple(theAnyField, std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class CP, class... Ws, std::enable_if_t<internal::is_hidden_column_of_vtab_v<CP, fts5>, bool> = true>
    constexpr internal::builtin_function_t<double, internal::bm25_string, CP, Ws...> bm25(CP theAnyField,
                                                                                          Ws... weights) {
        return {std::make_tuple(std::move(theAnyField), std::move(weights)...)};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class Hidden,
             class F,
             class... Ws,
             std::enable_if_t<internal::is_hidden_field_of_vtab_v<Hidden, F, fts5>, bool> = true>
    constexpr internal::builtin_function_t<double, internal::bm25_string, F Hidden::*, Ws...>
    bm25(F Hidden::* theAnyField, Ws... weights) {
        return {std::make_tuple(theAnyField, std::move(weights)...)};
    }

    /**
     *  The FTS5 snippet auxiliary function: returns the text of the column `columnIndex` with the matches
     *  surrounded by `matchOpen`/`matchClose`, truncated to at most `tokenCount` tokens,
     *  with `ellipses` marking the truncations.
     *  See https://www.sqlite.org/fts5.html#the_snippet_function
     */
    template<class CP,
             class X1,
             class X2,
             class X3,
             class X4,
             class X5,
             std::enable_if_t<internal::is_hidden_column_of_vtab_v<CP, fts5>, bool> = true>
    constexpr internal::builtin_function_t<std::string, internal::snippet_string, CP, X1, X2, X3, X4, X5>
    snippet(CP theAnyField, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(theAnyField),
                                std::move(columnIndex),
                                std::move(matchOpen),
                                std::move(matchClose),
                                std::move(ellipses),
                                std::move(tokenCount))};
    }

    /**
     *  The FTS5 snippet auxiliary function: returns the text of the column `columnIndex` with the matches
     *  surrounded by `matchOpen`/`matchClose`, truncated to at most `tokenCount` tokens,
     *  with `ellipses` marking the truncations.
     *  See https://www.sqlite.org/fts5.html#the_snippet_function
     */
    template<class Hidden,
             class F,
             class X1,
             class X2,
             class X3,
             class X4,
             class X5,
             std::enable_if_t<internal::is_hidden_field_of_vtab_v<Hidden, F, fts5>, bool> = true>
    constexpr internal::builtin_function_t<std::string, internal::snippet_string, F Hidden::*, X1, X2, X3, X4, X5>
    snippet(F Hidden::* theAnyField, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(theAnyField,
                                std::move(columnIndex),
                                std::move(matchOpen),
                                std::move(matchClose),
                                std::move(ellipses),
                                std::move(tokenCount))};
    }
#endif

    /**
     *  The FTS5 highlight function. The table type is specified as a template argument.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     *
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class O,
             class X,
             class Y,
             class Z,
             class VTab = fts5,
             std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    [[deprecated("Use the `highlight` function accepting the hidden FTS5 'any' field instead")]]
    constexpr auto highlight(X x, Y y, Z z) {
        //  the hidden column named like the table stands for the table;
        //  spelled through `VTab` so that `fts5` need only be complete where the function is instantiated
        return highlight(column<O>(&VTab::hidden::any), std::move(x), std::move(y), std::move(z));
    }
}
#endif
