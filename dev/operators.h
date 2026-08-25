#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits
#include "serialize_result_type.h"
#include "tags.h"
#include "vocabulary/algorithms/operand_predicates.h"

namespace sqlite_orm::internal {
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

    struct conc_string {
        serialize_result_type serialize() const {
            return "||";
        }
    };

    /**
     *  Result of concatenation || operator
     */
    template<class L, class R>
    using conc_t = binary_operator<L, R, conc_string>;

    template<class L, class R>
    constexpr bool is_chainable_operand_v<conc_t<L, R>> = true;

    struct unary_minus_string {
        serialize_result_type serialize() const {
            return "-";
        }
    };

    /**
     *  Result of unary minus - operator
     */
    template<class T>
    struct unary_minus_t : unary_minus_string, arithmetic_t, negatable_t {
        using argument_type = T;

        argument_type argument;

        unary_minus_t(argument_type argument_) : argument(std::move(argument_)) {}
    };

    struct add_string {
        serialize_result_type serialize() const {
            return "+";
        }
    };

    /**
     *  Result of addition + operator
     */
    template<class L, class R>
    using add_t = binary_operator<L, R, add_string, arithmetic_t, negatable_t>;

    struct sub_string {
        serialize_result_type serialize() const {
            return "-";
        }
    };

    /**
     *  Result of substraction - operator
     */
    template<class L, class R>
    using sub_t = binary_operator<L, R, sub_string, arithmetic_t, negatable_t>;

    struct mul_string {
        serialize_result_type serialize() const {
            return "*";
        }
    };

    /**
     *  Result of multiply * operator
     */
    template<class L, class R>
    using mul_t = binary_operator<L, R, mul_string, arithmetic_t, negatable_t>;

    struct div_string {
        serialize_result_type serialize() const {
            return "/";
        }
    };

    /**
     *  Result of divide / operator
     */
    template<class L, class R>
    using div_t = binary_operator<L, R, div_string, arithmetic_t, negatable_t>;

    struct mod_operator_string {
        serialize_result_type serialize() const {
            return "%";
        }
    };

    /**
     *  Result of mod % operator
     */
    template<class L, class R>
    using mod_t = binary_operator<L, R, mod_operator_string, arithmetic_t, negatable_t>;

    struct bitwise_shift_left_string {
        serialize_result_type serialize() const {
            return "<<";
        }
    };

    /**
     *  Result of bitwise shift left << operator
     */
    template<class L, class R>
    using bitwise_shift_left_t = binary_operator<L, R, bitwise_shift_left_string, arithmetic_t, negatable_t>;

    struct bitwise_shift_right_string {
        serialize_result_type serialize() const {
            return ">>";
        }
    };

    /**
     *  Result of bitwise shift right >> operator
     */
    template<class L, class R>
    using bitwise_shift_right_t = binary_operator<L, R, bitwise_shift_right_string, arithmetic_t, negatable_t>;

    struct bitwise_and_string {
        serialize_result_type serialize() const {
            return "&";
        }
    };

    /**
     *  Result of bitwise and & operator
     */
    template<class L, class R>
    using bitwise_and_t = binary_operator<L, R, bitwise_and_string, arithmetic_t, negatable_t>;

    struct bitwise_or_string {
        serialize_result_type serialize() const {
            return "|";
        }
    };

    /**
     *  Result of bitwise or | operator
     */
    template<class L, class R>
    using bitwise_or_t = binary_operator<L, R, bitwise_or_string, arithmetic_t, negatable_t>;

    struct bitwise_not_string {
        serialize_result_type serialize() const {
            return "~";
        }
    };

    /**
     *  Result of bitwise not ~ operator
     */
    template<class T>
    struct bitwise_not_t : bitwise_not_string, arithmetic_t, negatable_t {
        using argument_type = T;

        argument_type argument;

        bitwise_not_t(argument_type argument_) : argument(std::move(argument_)) {}
    };

    struct assign_string {
        serialize_result_type serialize() const {
            return "=";
        }
    };

    /**
     *  Result of assign = operator
     */
    template<class L, class R>
    using assign_t = binary_operator<L, R, assign_string>;

    template<class L, class R>
    constexpr bool is_assign_v<assign_t<L, R>> = true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Public interface for || concatenation operator. Example: `select(conc(&User::name, "@gmail.com"));` => SELECT
     *  name || '@gmail.com' FROM users
     */
    template<class L, class R>
    constexpr internal::conc_t<L, R> conc(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "conc() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class T>
    constexpr internal::unary_minus_t<T> minus(T t) {
        static_assert(internal::is_operand_or_bindable<T>::value,
                      "minus() argument must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(t)};
    }

    /**
     *  Public interface for + operator. Example: `select(add(&User::age, 100));` => SELECT age + 100 FROM users
     */
    template<class L, class R>
    constexpr internal::add_t<L, R> add(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "add() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    /**
     *  Public interface for - operator. Example: `select(sub(&User::age, 1));` => SELECT age - 1 FROM users
     */
    template<class L, class R>
    constexpr internal::sub_t<L, R> sub(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "sub() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    /**
     *  Public interface for * operator. Example: `select(mul(&User::salary, 2));` => SELECT salary * 2 FROM users
     */
    template<class L, class R>
    constexpr internal::mul_t<L, R> mul(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "mul() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    /**
     *  Public interface for / operator. Example: `select(div(&User::salary, 3));` => SELECT salary / 3 FROM users
     *  @note Please notice that ::div function already exists in pure C standard library inside <cstdlib> header.
     *  If you use `using namespace sqlite_orm` directive you an specify which `div` you call explicitly using  `::div` or `sqlite_orm::div` statements.
     */
    template<class L, class R>
    constexpr internal::div_t<L, R> div(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "div() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    /**
     *  Public interface for % operator. Example: `select(mod(&User::age, 5));` => SELECT age % 5 FROM users
     */
    template<class L, class R>
    constexpr internal::mod_t<L, R> mod(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "mod() arguments must be bindable values or sqlite_orm-recognized operands: member pointers, "
                      "column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::bitwise_shift_left_t<L, R> bitwise_shift_left(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "bitwise_shift_left() arguments must be bindable values or sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::bitwise_shift_right_t<L, R> bitwise_shift_right(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "bitwise_shift_right() arguments must be bindable values or sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::bitwise_and_t<L, R> bitwise_and(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "bitwise_and() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class L, class R>
    constexpr internal::bitwise_or_t<L, R> bitwise_or(L l, R r) {
        static_assert(internal::are_valid_operands<L, R>::value,
                      "bitwise_or() arguments must be bindable values or sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }

    template<class T>
    constexpr internal::bitwise_not_t<T> bitwise_not(T t) {
        static_assert(internal::is_operand_or_bindable<T>::value,
                      "bitwise_not() argument must be a bindable value or one of sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(t)};
    }

    template<class L, class R>
    internal::assign_t<L, R> assign(L l, R r) {
        static_assert(internal::is_referencable_operand<L>::value,
                      "the assignment target must be one of sqlite_orm-recognized operands: member pointers, column "
                      "pointers, c()-wrapped values, aliases or expressions");
        return {std::move(l), std::move(r)};
    }
}
