#pragma once

#include <type_traits>
#include <string>

#include "cxx_polyfill.h"
#include "alias.h"
#include "select_constraints.h"
#include "cte_types.h"

namespace sqlite_orm {

    /** 
     *  A special table alias that is both, a storage lookup type (mapping type) and an alias.
     */
    template<char C>
    struct cte_alias
        : internal::table_alias<cte_alias<C> /* refer to self, since a label is both, an alias and a lookup type */,
                                C> {};

    using cte_1 = cte_alias<'1'>;
    using cte_2 = cte_alias<'2'>;
    using cte_3 = cte_alias<'3'>;
    using cte_4 = cte_alias<'4'>;
    using cte_5 = cte_alias<'5'>;
    using cte_6 = cte_alias<'6'>;
    using cte_7 = cte_alias<'7'>;
    using cte_8 = cte_alias<'8'>;
    using cte_9 = cte_alias<'9'>;

#if __cplusplus >= 201703L  // use of C++17 or higher
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
     *  E.g. 0_col, 1_col
     */
    template<char... Chars>
    [[nodiscard]] constexpr decltype(auto) operator"" _col() {
        return polyfill::index_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{},
                                                                 Chars...)>{};
    }

    /**
     *  integral_constant<unsigned int> from numeric literal.
     *  E.g. 1_nth_col, 2_nth_col
     */
    template<char... Chars>
    [[nodiscard]] constexpr decltype(auto) operator"" _nth_col() {
        static_assert(((Chars != '0') && ...));
        return internal::nth_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{},
                                                               Chars...)>{};
    }
#else
    constexpr polyfill::index_constant<0> _0_col{};
    constexpr polyfill::index_constant<1> _1_col{};
    constexpr polyfill::index_constant<2> _2_col{};
    constexpr polyfill::index_constant<3> _3_col{};
    constexpr polyfill::index_constant<4> _4_col{};
    constexpr polyfill::index_constant<5> _5_col{};
    constexpr polyfill::index_constant<6> _6_col{};
    constexpr polyfill::index_constant<7> _7_col{};
    constexpr polyfill::index_constant<8> _8_col{};
    constexpr polyfill::index_constant<9> _9_col{};

    constexpr internal::nth_constant<1> _1_nth_col{};
    constexpr internal::nth_constant<2> _2_nth_col{};
    constexpr internal::nth_constant<3> _3_nth_col{};
    constexpr internal::nth_constant<4> _4_nth_col{};
    constexpr internal::nth_constant<5> _5_nth_col{};
    constexpr internal::nth_constant<6> _6_nth_col{};
    constexpr internal::nth_constant<7> _7_nth_col{};
    constexpr internal::nth_constant<8> _8_nth_col{};
    constexpr internal::nth_constant<9> _9_nth_col{};
    constexpr internal::nth_constant<10> _10_nth_col{};
#endif
}
