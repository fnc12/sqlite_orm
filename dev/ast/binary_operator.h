#pragma once

/** @file
 *  Purpose: give the binary operator nodes their shared shape.
 *
 *  Intention: a helper of the node headers in `ast/`, placed by `docs/internals/source-layout.md`; specializes the
 *  grammar traits of the vocabulary layer, as `docs/internals/vocabulary-layer.md` holds, for the nodes built on it.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../functional/cxx_polyfill/type_traits.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  L - type of the left operand
     *  R - type of the right operand
     *  Ds - the operator's tag, followed by further tags classifying the operator's operand capabilities
     */
    template<class L, class R, class... Ds>
    struct binary_operator : Ds... {
        using left_type = L;
        using right_type = R;

        left_type lhs;
        right_type rhs;

        constexpr binary_operator(left_type lhs_, right_type rhs_) : lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
    };

    template<class T>
    constexpr bool is_binary_operator_v = polyfill::is_specialization_of<T, binary_operator>::value;
}
