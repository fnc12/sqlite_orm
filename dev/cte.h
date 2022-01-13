#pragma once

#include <type_traits>
#include <string>

#include "cxx_polyfill.h"
#include "select_constraints.h"
#include "cte_types.h"

namespace sqlite_orm {

    template<char C, char... Chars>
    struct cte_label {
        static constexpr char str[] = {C, Chars..., '\0'};

        static constexpr const char* label() {
            return str;
        }

        explicit operator std::string() const {
            return cte_label::label();
        }
    };

    using cte_1 = cte_label<'c', 't', 'e', '_', '1'>;
    using cte_2 = cte_label<'c', 't', 'e', '_', '2'>;
    using cte_3 = cte_label<'c', 't', 'e', '_', '3'>;
    using cte_4 = cte_label<'c', 't', 'e', '_', '4'>;
    using cte_5 = cte_label<'c', 't', 'e', '_', '5'>;
    using cte_6 = cte_label<'c', 't', 'e', '_', '6'>;
    using cte_7 = cte_label<'c', 't', 'e', '_', '7'>;
    using cte_8 = cte_label<'c', 't', 'e', '_', '8'>;
    using cte_9 = cte_label<'c', 't', 'e', '_', '9'>;

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

    // index_constant<> from numeric literal
    template<char... Chars>
    [[nodiscard]] constexpr decltype(auto) operator"" _col() {
        return polyfill::index_constant<internal::n_from_literal(std::make_index_sequence<sizeof...(Chars)>{},
                                                                 Chars...)>{};
    }
#endif
}
