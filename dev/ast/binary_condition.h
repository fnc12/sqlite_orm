#pragma once

/** @file The binary conditions: the logical AND and OR, and the comparisons =, !=, IS, IS NOT,
 *        IS [NOT] DISTINCT FROM, >, >=, <, <= - with their factory functions and operator overloads.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <type_traits>  //  std::enable_if, std::disjunction, std::conjunction, std::negation
#include <utility>  //  std::move, std::forward
#include <sstream>  //  std::stringstream
#include <ostream>  //  std::flush
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/is_base_template_of.h"
#include "../builtin/collations.h"  //  collate_argument
#include "../tags.h"
#include "../table_reference.h"
#include "../alias_traits.h"
#include "../operators.h"  //  conc_t
#include "../vocabulary/node_algorithms.h"  // unwrap_expression, are_valid_operands
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "collate.h"

namespace sqlite_orm::internal {
    /**
     *  Base class for binary conditions
     *  L is left argument type
     *  R is right argument type
     *  S is 'string' class (a class which has cast to `std::string` operator)
     *  Res is result type
     */
    template<class L, class R, class S, class Res>
    struct binary_condition : condition_t, S {
        using left_type = L;
        using right_type = R;
        using result_type = Res;

        left_type lhs;
        right_type rhs;

        constexpr binary_condition() = default;

        constexpr binary_condition(left_type l_, right_type r_) : lhs(std::move(l_)), rhs(std::move(r_)) {}
    };

    template<class T>
    constexpr bool is_binary_condition_v = is_base_template_of_v<binary_condition, T>;

    struct and_condition_string {
        std::string_view serialize() const {
            return "AND";
        }
    };

    /**
     *  Result of and operator
     */
    template<class L, class R>
    struct and_condition_t : binary_condition<L, R, and_condition_string, bool>, negatable_t {
        using super = binary_condition<L, R, and_condition_string, bool>;

        using super::super;
    };

    struct or_condition_string {
        std::string_view serialize() const {
            return "OR";
        }
    };

    /**
     *  Result of or operator
     */
    template<class L, class R>
    struct or_condition_t : binary_condition<L, R, or_condition_string, bool>, negatable_t {
        using super = binary_condition<L, R, or_condition_string, bool>;

        using super::super;
    };

    struct is_equal_string {
        std::string_view serialize() const {
            return "=";
        }
    };

    /**
     *  = and == operators object
     */
    template<class L, class R>
    struct is_equal_t : binary_condition<L, R, is_equal_string, bool>, negatable_t {
        using binary_condition<L, R, is_equal_string, bool>::binary_condition;

        collate_t<is_equal_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<is_equal_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<is_equal_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }

        named_collate<is_equal_t> collate(std::string name) const {
            return {*this, std::move(name)};
        }

        template<class C>
        named_collate<is_equal_t> collate() const {
            std::stringstream ss;
            ss << C::name() << std::flush;
            return {*this, ss.str()};
        }
    };

    struct is_not_equal_string {
        std::string_view serialize() const {
            return "!=";
        }
    };

    /**
     *  != operator object
     */
    template<class L, class R>
    struct is_not_equal_t : binary_condition<L, R, is_not_equal_string, bool>, negatable_t {
        using binary_condition<L, R, is_not_equal_string, bool>::binary_condition;

        collate_t<is_not_equal_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<is_not_equal_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<is_not_equal_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct is_string {
        std::string_view serialize() const {
            return "IS";
        }
    };

    /**
     *  IS operator object
     */
    template<class L, class R>
    struct is_t : binary_condition<L, R, is_string, bool>, negatable_t {
        using binary_condition<L, R, is_string, bool>::binary_condition;
    };

    struct is_not_string {
        std::string_view serialize() const {
            return "IS NOT";
        }
    };

    /**
     *  IS NOT operator object
     */
    template<class L, class R>
    struct is_not_t : binary_condition<L, R, is_not_string, bool>, negatable_t {
        using binary_condition<L, R, is_not_string, bool>::binary_condition;
    };

#if SQLITE_VERSION_NUMBER >= 3039000
    struct is_distinct_from_string {
        std::string_view serialize() const {
            return "IS DISTINCT FROM";
        }
    };

    /**
     *  IS DISTINCT FROM operator object
     */
    template<class L, class R>
    struct is_distinct_from_t : binary_condition<L, R, is_distinct_from_string, bool>, negatable_t {
        using binary_condition<L, R, is_distinct_from_string, bool>::binary_condition;
    };

    struct is_not_distinct_from_string {
        std::string_view serialize() const {
            return "IS NOT DISTINCT FROM";
        }
    };

    /**
     *  IS NOT DISTINCT FROM operator object
     */
    template<class L, class R>
    struct is_not_distinct_from_t : binary_condition<L, R, is_not_distinct_from_string, bool>, negatable_t {
        using binary_condition<L, R, is_not_distinct_from_string, bool>::binary_condition;
    };
#endif

    struct greater_than_string {
        std::string_view serialize() const {
            return ">";
        }
    };

    /**
     *  > operator object.
     */
    template<class L, class R>
    struct greater_than_t : binary_condition<L, R, greater_than_string, bool>, negatable_t {
        using binary_condition<L, R, greater_than_string, bool>::binary_condition;

        collate_t<greater_than_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<greater_than_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<greater_than_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct greater_or_equal_string {
        std::string_view serialize() const {
            return ">=";
        }
    };

    /**
     *  >= operator object.
     */
    template<class L, class R>
    struct greater_or_equal_t : binary_condition<L, R, greater_or_equal_string, bool>, negatable_t {
        using binary_condition<L, R, greater_or_equal_string, bool>::binary_condition;

        collate_t<greater_or_equal_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<greater_or_equal_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<greater_or_equal_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct less_than_string {
        std::string_view serialize() const {
            return "<";
        }
    };

    /**
     *  < operator object.
     */
    template<class L, class R>
    struct less_than_t : binary_condition<L, R, less_than_string, bool>, negatable_t {
        using binary_condition<L, R, less_than_string, bool>::binary_condition;

        collate_t<less_than_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<less_than_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<less_than_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };

    struct less_or_equal_string {
        std::string_view serialize() const {
            return "<=";
        }
    };

    /**
     *  <= operator object.
     */
    template<class L, class R>
    struct less_or_equal_t : binary_condition<L, R, less_or_equal_string, bool>, negatable_t {
        using binary_condition<L, R, less_or_equal_string, bool>::binary_condition;

        collate_t<less_or_equal_t> collate_binary() const {
            return {*this, collate_argument::binary};
        }

        collate_t<less_or_equal_t> collate_nocase() const {
            return {*this, collate_argument::nocase};
        }

        collate_t<less_or_equal_t> collate_rtrim() const {
            return {*this, collate_argument::rtrim};
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    // Intentionally place operators for types classified as arithmetic or general operator arguments in the internal namespace
    // to facilitate ADL (Argument Dependent Lookup)
    namespace internal {
        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr less_than_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator<(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr less_or_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator<=(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr greater_than_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator>(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr greater_or_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator>=(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_conditional_operand<L>,
                                                   is_conditional_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value
#ifndef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
                                      && !is_table_reference_v<L>
#endif
                                  ,
                                  bool> = true>
        constexpr is_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator==(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_arithmetic_operand<L>,
                                                   is_arithmetic_operand<R>,
                                                   is_conditional_operand<L>,
                                                   is_conditional_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr is_not_equal_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator!=(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_conditional_operand<L>,
                                                   is_conditional_operand<R>,
                                                   is_operator_argument<L>,
                                                   is_operator_argument<R>>::value,
                                  bool> = true>
        constexpr and_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator&&(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        template<class L,
                 class R,
                 std::enable_if_t<std::disjunction<is_conditional_operand<L>, is_conditional_operand<R>>::value, bool> =
                     true>
        constexpr or_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator||(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }

        //  note: the string concatenation `||` lives next to the logical OR `||` it is told apart from
        template<class L,
                 class R,
                 std::enable_if_t<std::conjunction<std::disjunction<is_chainable_operand<L>,
                                                                    is_chainable_operand<R>,
                                                                    is_operator_argument<L>,
                                                                    is_operator_argument<R>>,
                                                   // exclude conditions
                                                   std::negation<std::disjunction<is_conditional_operand<L>,
                                                                                  is_conditional_operand<R>>>>::value,
                                  bool> = true>
        constexpr conc_t<unwrap_expression_t<L>, unwrap_expression_t<R>> operator||(L l, R r) {
            return {unwrap_expression(std::forward<L>(l)), unwrap_expression(std::forward<R>(r))};
        }
    }

    template<class L, class R>
    constexpr auto and_(L lhs, R rhs) {
        using namespace ::sqlite_orm::internal;
        static_assert(are_valid_operands<L, R>::value,
                      "and_() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return and_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>>{unwrap_expression(std::forward<L>(lhs)),
                                                                               unwrap_expression(std::forward<R>(rhs))};
    }

    template<class L, class R>
    constexpr auto or_(L lhs, R rhs) {
        using namespace ::sqlite_orm::internal;
        static_assert(are_valid_operands<L, R>::value,
                      "or_() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return or_condition_t<unwrap_expression_t<L>, unwrap_expression_t<R>>{unwrap_expression(std::forward<L>(lhs)),
                                                                              unwrap_expression(std::forward<R>(rhs))};
    }

    template<class L, class R>
    constexpr internal::is_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> is_equal(L lhs,
                                                                                                                R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is_equal() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::is_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> eq(L lhs,
                                                                                                          R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "eq() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::is_not_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    is_not_equal(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is_not_equal() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::is_not_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> ne(L lhs,
                                                                                                              R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "ne() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    /**
     *  IS operator: a NULL-safe equality comparison, `NULL IS NULL` evaluates to true.
     *  Example: storage.select(is(&User::middleName, std::nullopt))
     */
    template<class L, class R>
    constexpr internal::is_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> is(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    /**
     *  IS NOT operator: a NULL-safe inequality comparison, `1 IS NOT NULL` evaluates to true.
     *  Example: storage.select(is_not(&User::middleName, std::nullopt))
     */
    template<class L, class R>
    constexpr internal::is_not_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> is_not(L lhs,
                                                                                                            R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is_not() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

#if SQLITE_VERSION_NUMBER >= 3039000
    /**
     *  IS DISTINCT FROM operator: equivalent to IS NOT, for compatibility with PostgreSQL
     *  and the SQL standards.
     *  Example: storage.select(is_distinct_from(&User::middleName, &User::name))
     */
    template<class L, class R>
    constexpr internal::is_distinct_from_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    is_distinct_from(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is_distinct_from() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    /**
     *  IS NOT DISTINCT FROM operator: equivalent to IS, for compatibility with PostgreSQL
     *  and the SQL standards.
     *  Example: storage.select(is_not_distinct_from(&User::middleName, &User::name))
     */
    template<class L, class R>
    constexpr internal::is_not_distinct_from_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    is_not_distinct_from(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "is_not_distinct_from() arguments must be bindable values or sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }
#endif

    template<class L, class R>
    constexpr internal::greater_than_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    greater_than(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "greater_than() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::greater_than_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> gt(L lhs,
                                                                                                              R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "gt() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::greater_or_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    greater_or_equal(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "greater_or_equal() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::greater_or_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    ge(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "ge() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::less_than_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    less_than(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "less_than() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    /**
     *  [Deprecation notice] This function is deprecated and will be removed in v1.10. Use the accurately named function `less_than(...)` instead.
     */
    template<class L, class R>
    [[deprecated("Use the accurately named function `less_than(...)` instead")]] internal::
        less_than_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
        lesser_than(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "lesser_than() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::less_than_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> lt(L lhs,
                                                                                                           R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "lt() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::less_or_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
    less_or_equal(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "less_or_equal() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    /**
     *  [Deprecation notice] This function is deprecated and will be removed in v1.10. Use the accurately named function `less_or_equal(...)` instead.
     */
    template<class L, class R>
    [[deprecated("Use the accurately named function `less_or_equal(...)` instead")]] internal::
        less_or_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>>
        lesser_or_equal(L lhs, R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "lesser_or_equal() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }

    template<class L, class R>
    constexpr internal::less_or_equal_t<internal::unwrap_expression_t<L>, internal::unwrap_expression_t<R>> le(L lhs,
                                                                                                               R rhs) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "le() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(lhs)), internal::unwrap_expression(std::move(rhs))};
    }
}
