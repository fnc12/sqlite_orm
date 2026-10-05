#pragma once

/** @file The deprecated FTS5 spellings: `order_by(rank())` for the hidden FTS5 rank column, and `is_equal<Table>(x)`
 *        for the comparison of a whole FTS5 table with an expression.
 *
 *        `rank()` is the RANK() window function, and it happened to be the spelling of the hidden FTS5 rank
 *        column as well, by the bare `rank` it used to serialize to. That dual meaning gets a header of its own,
 *        which pulls in what it needs rather than having the FTS5 module, the window functions or ORDER BY
 *        depend on it.
 *
 *        `is_equal<Table>(x)` compares the table itself, which is not a binary condition: its left side is a type
 *        only. Unlike the rank spelling, it is not guarded by FTS5 support, as its classification trait is read by
 *        the serializer, the AST iterator and the node tuple in every build.
 *
 *        Goes as a whole in v1.11.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
#include <string_view>  //  std::string_view
#endif
#endif

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_type_traits_polyfill.h"
#include "../../tags.h"  //  negatable_t
#include "../../alias_traits.h"  //  is_recordset_alias_v
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../functions/window.h"  //  rank
#include "../../ast/order_by.h"  //  order_by_t
#include "../../statement_serializer.h"  //  statement_serializer

namespace sqlite_orm::internal {
    /**
     *  The deprecated comparison of a whole FTS5 table with an expression: table = expression.
     */
    template<class L, class R>
    struct is_equal_with_table_t : negatable_t {
        using left_type = L;
        using right_type = R;

        right_type rhs;

        is_equal_with_table_t(right_type rhs) : rhs(std::move(rhs)) {}
    };

    template<class T>
    constexpr bool is_equal_with_table_v = polyfill::is_specialization_of_v<T, is_equal_with_table_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    template<class O, class R, std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    [[deprecated("Use the usual `is_equal` function to compare the hidden FTS5 'any' field or a field of your FTS "
                 "table instead")]]
    constexpr internal::is_equal_with_table_t<O, R> is_equal(R rhs) {
        return {std::move(rhs)};
    }
}

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
namespace sqlite_orm::internal {
    /*
     *  The hidden FTS5 rank column as the deprecated `order_by(rank())` refers to it, i.e. not bound to a table.
     */
    struct fts5_rank_column_t {};

    template<>
    struct statement_serializer<fts5_rank_column_t, void> {
        using statement_type = fts5_rank_column_t;

        template<class Ctx>
        SQLITE_ORM_STATIC_CALLOP std::string_view operator()(const statement_type& /*statement*/,
                                                             const Ctx&) SQLITE_ORM_OR_CONST_CALLOP {
            return "rank";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  [Deprecation notice] This expression factory function is deprecated and will be removed in v1.11.
     */
    [[deprecated("Use the hidden FTS5 rank column instead")]]
    inline internal::order_by_t<internal::fts5_rank_column_t> order_by(decltype(rank()) /*rank*/) {
        return {internal::fts5_rank_column_t{}};
    }
}
#endif
