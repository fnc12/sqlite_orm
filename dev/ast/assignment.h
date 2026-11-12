#pragma once

/** @file
 *  Purpose: represent the assignment of a SET clause.
 *
 *  Intention: a node header of `ast/`, placed by `docs/internals/source-layout.md`; specializes the vocabulary axes
 *  in place, as `docs/internals/vocabulary-layer.md` holds, and is gathered by `node_definitions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#include <type_traits>  //  std::is_base_of
#include <utility>  //  std::move
#endif

#include "../vocabulary/node_algorithms.h"  // is_referencable_operand, unwrap_expression
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "binary_operator.h"

namespace sqlite_orm::internal {
    struct assign_string {
        std::string_view serialize() const {
            return "=";
        }
    };

    /**
     *  Result of assign = operator
     */
    template<class L, class R>
    using assign_t = binary_operator<L, R, assign_string>;

    //  note: see `is_conc_v` of `operators.h`
    template<class T>
    constexpr bool is_assign_v = is_binary_operator_v<T> && std::is_base_of<assign_string, T>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class L, class R>
    internal::assign_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> assign(L lhs, R rhs) {
        static_assert(internal::is_referencable_operand<L>::value,
                      "the assignment target must be one of sqlite_orm-recognized operands: member pointers, column "
                      "pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }
}
