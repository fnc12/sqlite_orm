#pragma once

/** @file The pattern matching operators LIKE (with an optional ESCAPE) and GLOB.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../optional_container.h"
#include "../tags.h"
#include "../vocabulary/node_algorithms.h"  // is_operand_or_bindable
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    struct like_string {
        operator std::string() const {
            return "LIKE";
        }
    };

    /**
     *  LIKE operator object.
     */
    template<class A, class T, class E>
    struct like_t : condition_t, like_string, negatable_t {
        using expression_type = A;
        using pattern_type = T;
        using escape_type = E;

        expression_type _arg;
        pattern_type _pattern;
        optional_container<escape_type> _escape;  //  not escape cause escape exists as a function here

        constexpr like_t(expression_type arg_, pattern_type pattern_, optional_container<escape_type> escape_) :
            _arg(std::move(arg_)), _pattern(std::move(pattern_)), _escape(std::move(escape_)) {}

        template<class C>
        constexpr like_t<A, T, C> escape(C c) && {
            return {std::move(this->_arg), std::move(this->_pattern), {std::move(c)}};
        }
    };

    template<class T>
    constexpr bool is_like_v = polyfill::is_specialization_of_v<T, like_t>;

    struct glob_string {
        operator std::string() const {
            return "GLOB";
        }
    };

    template<class A, class T>
    struct glob_t : condition_t, glob_string, negatable_t {
        using expression_type = A;
        using pattern_type = T;

        expression_type arg;
        pattern_type pattern;

        constexpr glob_t(expression_type arg_, pattern_type pattern_) :
            arg(std::move(arg_)), pattern(std::move(pattern_)) {}
    };

    template<class T>
    constexpr bool is_glob_v = polyfill::is_specialization_of_v<T, glob_t>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  X LIKE Y
     *  Example: storage.select(like(&User::name, "T%"))
     */
    template<class A, class T>
    constexpr internal::like_t<A, T, void> like(A expression, T pattern) {
        static_assert(internal::is_operand_or_bindable<A>::value,
                      "the matched expression must be a bindable value or one of sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(expression), std::move(pattern), {}};
    }

    /**
     *  X LIKE Y ESCAPE Z
     *  Example: storage.select(like(&User::name, "T%", "%"))
     */
    template<class A, class T, class E>
    constexpr internal::like_t<A, T, E> like(A expression, T pattern, E escape) {
        static_assert(internal::is_operand_or_bindable<A>::value,
                      "the matched expression must be a bindable value or one of sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(expression), std::move(pattern), {std::move(escape)}};
    }

    /**
     *  X GLOB Y
     *  Example: storage.select(glob(&User::name, "*S"))
     */
    template<class A, class T>
    constexpr internal::glob_t<A, T> glob(A expression, T pattern) {
        static_assert(internal::is_operand_or_bindable<A>::value,
                      "the matched expression must be a bindable value or one of sqlite_orm-recognized operands: "
                      "member pointers, column pointers, c()-wrapped values, aliases or expressions");
        return {std::move(expression), std::move(pattern)};
    }
}
