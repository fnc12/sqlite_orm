#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <type_traits>  //  std::enable_if, std::disjunction
#include <utility>  //  std::move, std::forward
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tags.h"
#include "../vocabulary/node_algorithms.h"  //  unwrap_expression
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  //  is_negatable_operand, is_operator_argument

namespace sqlite_orm::internal {
    struct negated_condition_string {
        operator std::string() const {
            return "NOT";
        }
    };

    /**
     *  Result of not operator
     */
    template<class C>
    struct negated_condition_t : condition_t, negated_condition_string {
        using argument_type = C;

        argument_type c;

        constexpr negated_condition_t(argument_type arg) : c(std::move(arg)) {}
    };

    template<class T>
    constexpr bool is_negated_condition_v = polyfill::is_specialization_of_v<T, negated_condition_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    // Intentionally place operators for types classified as arithmetic or general operator arguments in the internal namespace
    // to facilitate ADL (Argument Dependent Lookup)
    namespace internal {
        template<
            class T,
            std::enable_if_t<std::disjunction<is_negatable_operand<T>, is_operator_argument<T>>::value, bool> = true>
        constexpr negated_condition_t<unwrap_expression_t<T>> operator!(T arg) {
            return {unwrap_expression(std::forward<T>(arg))};
        }
    }
}
