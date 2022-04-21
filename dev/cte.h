#pragma once

#include <type_traits>
#include <string>

#include "cxx_polyfill.h"
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

#if __cplusplus >= 201703L  // C++17 or later
    namespace internal {
        constexpr size_t _10_pow(size_t n) {
            if(n == 0) {
                return 1;
            } else {
                return 10 * _10_pow(n - 1);
            }
        }

        template<class... Chars, size_t... Is>
        constexpr size_t n_from_literal(std::index_sequence<Is...>, Chars... chars) {
            return (((chars - '0') * _10_pow(sizeof...(Is) - 1u - Is /*reversed index sequence*/)) + ...);
        }
    }

    /**
     *  index_constant<> from numeric literal.
     *  E.g. 0_colidx, 1_colidx
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL decltype(auto) operator"" _colidx() {
        return polyfill::index_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{},
                                                                 Chars...)>{};
    }

    /**
     *  integral_constant<unsigned int> from numeric literal.
     *  E.g. 1_nth_col, 2_nth_col
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL decltype(auto) operator"" _nth_col() {
        constexpr auto n =
            internal::nth_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{}, Chars...)>{};
        static_assert(n > 0);
        return n;
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

#if __cplusplus >= 202002L  // C++20 or later
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
#endif
}
