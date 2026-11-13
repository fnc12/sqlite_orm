#pragma once

/** @file
 *  Purpose: keep apart the deprecated FTS5 spellings that overload a general factory of the DSL.
 *
 *  Intention: gathered by `builtin/dbos.h`; keeps `fts5.h` free of what they need, `ast/order_by.h`,
 *  `functions/window.h` and the statement serializer, and each factory says when it goes.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_FTS5_SUPPORTED
#include <string_view>  //  std::string_view
#endif
#endif

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <utility>  //  std::move
#endif

#include "../../functional/cxx_polyfill/type_traits.h"
#include "../../ast/operand_tags.h"  //  negatable_t
#include "../../ast/order_by.h"  //  order_by_t
#include "../../serialization/statement_serializer.h"  //  statement_serializer
#include "../../vocabulary/node_traits.h"  //  is_recordset_alias_v
#include "../../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../functions/window.h"  //  rank

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
     *
     *  It compares the table itself, whose left side is a type only, so it builds no binary condition. It stays
     *  outside the FTS5 guard: the serializer, the AST iterator and the node tuple read its trait in every build.
     */
    template<class O, class R, std::enable_if_t<!internal::is_recordset_alias_v<O>, bool> = true>
    [[deprecated("Use the usual `is_equal` function to compare the hidden FTS5 'any' field or a field of your FTS "
                 "table instead")]]
    constexpr internal::is_equal_with_table_t<O, R> is_equal(R rhs) {
        return {std::move(rhs)};
    }
}

#ifdef SQLITE_ORM_FTS5_SUPPORTED
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
     *
     *  `rank()` is the RANK() window function; it spelled the hidden FTS5 rank column too, through the bare `rank`
     *  it serialized to.
     */
    [[deprecated("Use the hidden FTS5 rank column instead")]]
    inline internal::order_by_t<internal::fts5_rank_column_t> order_by(decltype(rank()) /*rank*/) {
        return {internal::fts5_rank_column_t{}};
    }
}
#endif
