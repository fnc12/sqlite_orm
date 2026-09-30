#pragma once

/** @file The FROM clause, in both of the DSL spellings sqlite_orm offers for it - naming mapped tables by type,
 *        or listing table expressions, such as a table-valued function.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::disjunction
#include <tuple>  //  std::tuple
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../table_reference.h"
#include "../alias_traits.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class... Tables>
    struct from_t {
        using tuple_type = std::tuple<Tables...>;
    };

    template<class T>
    constexpr bool is_from_v = polyfill::is_specialization_of_v<T, from_t>;

    template<class... TableExpr>
    struct from2_t {
        using tuple_type = std::tuple<TableExpr...>;

        tuple_type table_expressions;
    };

    template<class T>
    constexpr bool is_from2_v = polyfill::is_specialization_of_v<T, from2_t>;

    template<class T>
    constexpr bool is_any_from_v = std::disjunction_v<is_from<T>, is_from2<T>>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Explicit FROM function. Usage:
     *  `storage.select(&User::id, from<User>());`
     */
    template<class... Tables>
    constexpr internal::from_t<Tables...> from() {
        static_assert(sizeof...(Tables) > 0);
        return {};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Explicit FROM function. Usage:
     *  `storage.select(&User::id, from<"a"_alias.for_<User>>());`
     */
    template<orm_refers_to_recordset auto... recordsets>
    constexpr auto from() {
        return from<internal::auto_decay_table_ref_t<recordsets>...>();
    }
#endif

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /**
     *  Explicit FROM for an eponymous virtual table used as a table-valued function. Usage:
     *  `storage.select(asterisk<dbstat>(), from(dbstat_table("main", true)));`
     */
    template<class... TableExpr>
        requires ((orm_refers_to_recordset<TableExpr> || orm_table_valued_expression<TableExpr>) && ...)
    constexpr internal::from2_t<TableExpr...> from(TableExpr... tableExpressions) {
        return {{std::move(tableExpressions)...}};
    }
#else
    /**
     *  Explicit FROM for an eponymous virtual table used as a table-valued function. Usage:
     *  `storage.select(asterisk<dbstat>(), from(dbstat_table("main", true)));`
     */
    template<class... TableExpr>
    constexpr internal::from2_t<TableExpr...> from(TableExpr... tableExpressions) {
        static_assert(
            ((internal::is_referring_to_recordset_v<TableExpr> || internal::is_table_valued_expression_v<TableExpr>) &&
             ...));
        return {{std::move(tableExpressions)...}};
    }
#endif
}
