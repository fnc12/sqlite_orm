#pragma once

/** @file The FTS5 auxiliary functions `highlight()`, `bm25()` and `snippet()`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <tuple>  //  std::tuple, std::tuple_size, std::make_tuple
#include <utility>  //  std::move
#include <type_traits>  //  std::enable_if
#endif

#include <sqlite3.h>

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/type_traits.h"
#include "../alias_traits.h"
#include "../vocabulary/node_algorithms.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

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

    /*
     *  An FTS5 auxiliary function: a function whose first argument in SQL is the FTS5 table itself,
     *  which is identified by the mapped object type `T` and serialized as the looked-up table name.
     *  Deliberately not a `builtin_function_t`, so that the general built-in function machinery,
     *  which knows nothing about the table argument, never matches it.
     */
    template<class R, class S, class T, class... Args>
    struct fts5_auxiliary_function_t : S {
        using return_type = R;
        using string_type = S;
        using table_type = T;
        using args_type = std::tuple<Args...>;

        static constexpr size_t args_size = std::tuple_size<args_type>::value;

        args_type args;

        constexpr fts5_auxiliary_function_t(args_type args) : args(std::move(args)) {}
    };

    template<class T>
    constexpr bool is_fts_auxiliary_function_v = polyfill::is_specialization_of_v<T, fts5_auxiliary_function_t>;

    template<class T, class X, class Y, class Z>
    using highlight_t = fts5_auxiliary_function_t<std::string, highlight_string, T, X, Y, Z>;
}

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
SQLITE_ORM_EXPORT namespace sqlite_orm {
    struct fts5;

    /**
     *  The FTS5 highlight function. The table type is specified as a template argument.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     *
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class O, class X, class Y, class Z, std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    [[deprecated("Use the `highlight` function accepting the hidden FTS5 'any' field instead")]]
    constexpr internal::highlight_t<O, X, Y, Z> highlight(X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class CP, class X, class Y, class Z>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::highlight_t<internal::type_t<CP>, X, Y, Z> highlight(const CP& /*theAnyField*/, X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 highlight function.
     *  See https://www.sqlite.org/fts5.html#the_highlight_function
     */
    template<class Hidden, class F, class X, class Y, class Z>
        requires (internal::hidden_field_of_vtab<Hidden, F, fts5>)
    constexpr internal::highlight_t<typename Hidden::enclosing_type, X, Y, Z>
    highlight(F Hidden::* /*theAnyField*/, X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class CP, class... Ws>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::fts5_auxiliary_function_t<double, internal::bm25_string, internal::type_t<CP>, Ws...>
    bm25(const CP& /*theAnyField*/, Ws... weights) {
        return {std::make_tuple(std::move(weights)...)};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class Hidden, class F, class... Ws>
        requires (internal::hidden_field_of_vtab<Hidden, F, fts5>)
    constexpr internal::fts5_auxiliary_function_t<double, internal::bm25_string, typename Hidden::enclosing_type, Ws...>
    bm25(F Hidden::* /*theAnyField*/, Ws... weights) {
        return {std::make_tuple(std::move(weights)...)};
    }

    /**
     *  The FTS5 snippet auxiliary function: returns the text of the column `columnIndex` with the matches
     *  surrounded by `matchOpen`/`matchClose`, truncated to at most `tokenCount` tokens,
     *  with `ellipses` marking the truncations.
     *  See https://www.sqlite.org/fts5.html#the_snippet_function
     */
    template<class CP, class X1, class X2, class X3, class X4, class X5>
        requires (internal::hidden_column_of_vtab<CP, fts5>)
    constexpr internal::
        fts5_auxiliary_function_t<std::string, internal::snippet_string, internal::type_t<CP>, X1, X2, X3, X4, X5>
        snippet(const CP& /*theAnyField*/, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(columnIndex),
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
    constexpr internal::fts5_auxiliary_function_t<std::string,
                                                  internal::snippet_string,
                                                  typename Hidden::enclosing_type,
                                                  X1,
                                                  X2,
                                                  X3,
                                                  X4,
                                                  X5>
    snippet(F Hidden::* /*theAnyField*/, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(columnIndex),
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
    constexpr internal::highlight_t<internal::type_t<CP>, X, Y, Z> highlight(const CP& /*theAnyField*/, X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
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
    constexpr internal::highlight_t<typename Hidden::enclosing_type, X, Y, Z>
    highlight(F Hidden::* /*theAnyField*/, X x, Y y, Z z) {
        return {std::make_tuple(std::move(x), std::move(y), std::move(z))};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class CP, class... Ws, std::enable_if_t<internal::is_hidden_column_of_vtab_v<CP, fts5>, bool> = true>
    constexpr internal::fts5_auxiliary_function_t<double, internal::bm25_string, internal::type_t<CP>, Ws...>
    bm25(const CP& /*theAnyField*/, Ws... weights) {
        return {std::make_tuple(std::move(weights)...)};
    }

    /**
     *  The FTS5 bm25 auxiliary function. Optionally takes one weight per mapped table column.
     *  See https://www.sqlite.org/fts5.html#the_bm25_function
     */
    template<class Hidden,
             class F,
             class... Ws,
             std::enable_if_t<internal::is_hidden_field_of_vtab_v<Hidden, F, fts5>, bool> = true>
    constexpr internal::fts5_auxiliary_function_t<double, internal::bm25_string, typename Hidden::enclosing_type, Ws...>
    bm25(F Hidden::* /*theAnyField*/, Ws... weights) {
        return {std::make_tuple(std::move(weights)...)};
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
    constexpr internal::
        fts5_auxiliary_function_t<std::string, internal::snippet_string, internal::type_t<CP>, X1, X2, X3, X4, X5>
        snippet(const CP& /*theAnyField*/, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(columnIndex),
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
    constexpr internal::fts5_auxiliary_function_t<std::string,
                                                  internal::snippet_string,
                                                  typename Hidden::enclosing_type,
                                                  X1,
                                                  X2,
                                                  X3,
                                                  X4,
                                                  X5>
    snippet(F Hidden::* /*theAnyField*/, X1 columnIndex, X2 matchOpen, X3 matchClose, X4 ellipses, X5 tokenCount) {
        return {std::make_tuple(std::move(columnIndex),
                                std::move(matchOpen),
                                std::move(matchClose),
                                std::move(ellipses),
                                std::move(tokenCount))};
    }
#endif
}
#endif
