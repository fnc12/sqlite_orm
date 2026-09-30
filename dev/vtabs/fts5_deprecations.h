#pragma once

/** @file The deprecated `order_by(rank())` spelling of the hidden FTS5 rank column.
 *
 *        `rank()` is the RANK() window function, and it happened to be the spelling of the hidden FTS5 rank
 *        column as well, by the bare `rank` it used to serialize to. That dual meaning gets a header of its own,
 *        which pulls in what it needs rather than having the FTS5 module, the window functions or ORDER BY
 *        depend on it. Goes as a whole in v1.11.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
#include <string_view>  //  std::string_view
#endif
#endif

#include "../window_functions.h"  //  rank
#include "../conditions.h"  //  order_by_t
#include "../statement_serializer.h"  //  statement_serializer

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
