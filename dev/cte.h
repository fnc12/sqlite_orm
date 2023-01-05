#pragma once

#include <utility>  //  std::make_index_sequence

#include "alias.h"

namespace sqlite_orm {

    namespace internal {
        /** 
         *  A special table alias that is both, a storage lookup type (mapping type) and an alias.
         */
        template<char C, char... X>
        struct cte_alias
            : table_alias<cte_alias<C, X...> /* refer to self, since a label is both, an alias and a lookup type */,
                          C,
                          X...> {};
    }

    /**
     *  cte_alias<'n'> from a numeric literal.
     *  E.g. 1_ctealias, 2_ctealias
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL auto operator"" _ctealias() {
        return internal::cte_alias<Chars...>{};
    }
#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    /**
     *  cte_alias<'1'[, ...]> from a string literal.
     *  E.g. "1"_cte, "2"_cte
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _cte() {
        static_assert(t.size() != 1 || ((t.id[0] < 'A' || 'Z' < t.id[0]) && (t.id[0] < 'a' || 'z' < t.id[0])),
                      "CTE alias identifiers consisting of a single alphabetic character should be avoided, in order "
                      "to evade clashes with the built-in table aliases.");
        return internal::to_alias<internal::cte_alias, t>(std::make_index_sequence<t.size()>{});
    }
#endif

#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
    using cte_1 = decltype("1"_cte);
    using cte_2 = decltype("2"_cte);
    using cte_3 = decltype("3"_cte);
    using cte_4 = decltype("4"_cte);
    using cte_5 = decltype("5"_cte);
    using cte_6 = decltype("6"_cte);
    using cte_7 = decltype("7"_cte);
    using cte_8 = decltype("8"_cte);
    using cte_9 = decltype("9"_cte);
#else
    using cte_1 = decltype(1_ctealias);
    using cte_2 = decltype(2_ctealias);
    using cte_3 = decltype(3_ctealias);
    using cte_4 = decltype(4_ctealias);
    using cte_5 = decltype(5_ctealias);
    using cte_6 = decltype(6_ctealias);
    using cte_7 = decltype(7_ctealias);
    using cte_8 = decltype(8_ctealias);
    using cte_9 = decltype(9_ctealias);
#endif
}
