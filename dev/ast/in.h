#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <initializer_list>  //  std::initializer_list
#include <tuple>  //  std::tuple
#include <type_traits>  //  std::disjunction
#include <utility>  //  std::move
#include <vector>  //  std::vector
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tags.h"
#include "../vocabulary/node_algorithms.h"  // is_operand_or_bindable
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {

    struct in_base {
        bool negative = false;  //  used in not_in
    };

    /**
     *  IN operator object.
     */
    template<class L, class A>
    struct dynamic_in_t : condition_t, in_base, negatable_t {
        using left_type = L;
        using argument_type = A;

        left_type left;  //  left expression
        argument_type argument;  //  in arg

        dynamic_in_t(left_type left_, argument_type argument_, bool negative_) :
            in_base{negative_}, left(std::move(left_)), argument(std::move(argument_)) {}
    };

    template<class T>
    constexpr bool is_dynamic_in_v = polyfill::is_specialization_of_v<T, dynamic_in_t>;

    template<class L, class... Args>
    struct in_t : condition_t, in_base, negatable_t {
        using left_type = L;
        using argument_type = std::tuple<Args...>;

        left_type left;
        argument_type argument;

        in_t(left_type left_, argument_type argument_, bool negative_) :
            in_base{negative_}, left(std::move(left_)), argument(std::move(argument_)) {}
    };

    template<class T>
    constexpr bool is_in_v = polyfill::is_specialization_of_v<T, in_t>;

    template<class T>
    constexpr bool is_any_in_v = std::disjunction<is_in<T>, is_dynamic_in<T>>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  IN operator with vector of values.
     *  Example: in(&User::id, std::vector<int>{1, 2, 3})
     *  @param left Left expression (column or value to check).
     *  @param values Vector of values to check against.
     *  @return dynamic_in_t instance representing IN clause.
     */
    template<class L, class E>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, std::vector<E>> in(L left, std::vector<E> values) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(values), false};
    }

    /**
     *  IN operator with initializer list.
     *  Example: in(&User::id, {1, 2, 3})
     *  @param left Left expression (column or value to check).
     *  @param values Initializer list of values to check against.
     *  @return dynamic_in_t instance representing IN clause.
     */
    template<class L, class E>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, std::vector<E>> in(L left,
                                                                                std::initializer_list<E> values) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(values), false};
    }

    /**
     *  IN operator with a subquery or custom argument.
     *  Example: in(&User::id, select(&Employee::managerId))
     *  @param left Left expression (column or value to check).
     *  @param argument Subquery or container to check against.
     *  @return dynamic_in_t instance representing IN clause.
     */
    template<class L, class A>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, A> in(L left, A argument) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(argument), false};
    }

    /**
     *  NOT IN operator with vector of values.
     *  Example: not_in(&User::id, std::vector<int>{1, 2, 3})
     *  @param left Left expression (column or value to check).
     *  @param values Vector of values to check against.
     *  @return dynamic_in_t instance representing NOT IN clause.
     */
    template<class L, class E>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, std::vector<E>> not_in(L left, std::vector<E> values) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(values), true};
    }

    /**
     *  NOT IN operator with initializer list.
     *  Example: not_in(&User::id, {1, 2, 3})
     *  @param left Left expression (column or value to check).
     *  @param values Initializer list of values to check against.
     *  @return dynamic_in_t instance representing NOT IN clause.
     */
    template<class L, class E>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, std::vector<E>> not_in(L left,
                                                                                    std::initializer_list<E> values) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(values), true};
    }

    /**
     *  NOT IN operator with a subquery or custom argument.
     *  Example: not_in(&User::id, select(&Employee::managerId))
     *  @param left Left expression (column or value to check).
     *  @param argument Subquery or container to check against.
     *  @return dynamic_in_t instance representing NOT IN clause.
     */
    template<class L, class A>
    internal::dynamic_in_t<internal::unwrap_expression_t<L>, A> not_in(L left, A argument) {
        static_assert(internal::is_operand_or_bindable<L>::value,
                      "the tested expression must be a bindable value or one of sqlite_orm-recognized operands: member "
                      "pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {internal::unwrap_expression(std::move(left)), std::move(argument), true};
    }
}