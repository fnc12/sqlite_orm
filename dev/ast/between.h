#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tags.h"
#include "../vocabulary/node_algorithms.h"  // is_operand_or_bindable
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  BETWEEN operator object.
     */
    template<class A, class T>
    struct between_t : condition_t, negatable_t {
        using expression_type = A;
        using lower_type = T;
        using upper_type = T;

        expression_type expression;
        lower_type lower;
        upper_type upper;

        between_t(expression_type expression_, lower_type lower_, upper_type upper_) :
            expression(std::move(expression_)), lower(std::move(lower_)), upper(std::move(upper_)) {}
    };

    template<class T>
    constexpr bool is_between_v = polyfill::is_specialization_of_v<T, between_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  X BETWEEN Y AND Z
     *  Example: storage.select(between(&User::id, 10, 20))
     */
    template<class A, class T>
    internal::between_t<internal::unwrap_expression_t<A>, internal::unwrap_expression_t<T>>
    between(A expression, T lower, T upper) {
        static_assert(internal::is_operand_or_bindable<A>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(expression)),
                internal::unwrap_expression(std::move(lower)),
                internal::unwrap_expression(std::move(upper))};
    }
}