#pragma once

#ifdef SQLITE_ORM_WITH_CTE
#include <utility>  //  std::make_index_sequence
#endif

#include "functional/cxx_universal.h"
#include "alias.h"

#ifdef SQLITE_ORM_WITH_CTE
namespace sqlite_orm {

    namespace internal {
        /** 
         *  A special record set alias that is both, a storage lookup type (mapping type) and an alias.
         */
        template<char A, char... X>
        struct cte_moniker
            : recordset_alias<
                  cte_moniker<A, X...> /* refer to self, since a moniker is both, an alias and a mapped type */,
                  A,
                  X...> {};
    }

    /**
     *  cte_moniker<'n'> from a numeric literal.
     *  E.g. 1_ctealias, 2_ctealias
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL auto operator"" _ctealias() {
        return internal::cte_moniker<Chars...>{};
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  cte_moniker<'1'[, ...]> from a string literal.
     *  E.g. "1"_cte, "2"_cte
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _cte() {
        return internal::to_alias<internal::cte_moniker, t>(std::make_index_sequence<t.size()>{});
    }
#endif
}
#endif
