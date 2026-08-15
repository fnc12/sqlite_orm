#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <utility>  //  std::move, std::forward, std::declval, std::forward_like
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "in.h"
#include "../conditions.h"
#include "../operators.h"
#include "../vocabulary/traits/structural_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  Result of c(...) function. Has operator= overloaded which returns assign_t
     */
    template<class T>
    struct quoted_expression_t {
        T _value;

#if defined(SQLITE_ORM_DEDUCING_THIS_SUPPORTED) && __cpp_lib_forward_like >= 202207L
        template<class Self, class R>
        assign_t<T, R> operator=(this Self&& self, R right) {
            return {std::forward_like<Self>(self._value), std::move(right)};
        }

        template<class Self, class... Args>
        in_t<T, Args...> in(this Self&& self, Args... args) {
            return {std::forward_like<Self>(self._value), {std::forward<Args>(args)...}, false};
        }

        template<class Self, class... Args>
        in_t<T, Args...> not_in(this Self&& self, Args... args) {
            return {std::forward_like<Self>(self._value), {std::forward<Args>(args)...}, true};
        }

        template<class Self, class R>
        and_condition_t<T, R> and_(this Self&& self, R right) {
            return {std::forward_like<Self>(self._value), std::move(right)};
        }

        template<class Self, class R>
        or_condition_t<T, R> or_(this Self&& self, R right) {
            return {std::forward_like<Self>(self._value), std::move(right)};
        }
#else
        template<class R>
        assign_t<T, R> operator=(R right) const {
            return {_value, std::move(right)};
        }

        template<class... Args>
        in_t<T, Args...> in(Args... args) const {
            return {_value, {std::forward<Args>(args)...}, false};
        }

        template<class... Args>
        in_t<T, Args...> not_in(Args... args) const {
            return {_value, {std::forward<Args>(args)...}, true};
        }

        template<class R>
        and_condition_t<T, R> and_(R right) const {
            return {_value, std::move(right)};
        }

        template<class R>
        or_condition_t<T, R> or_(R right) const {
            return {_value, std::move(right)};
        }
#endif
    };

    template<class T>
    constexpr bool is_quoted_expression_v = polyfill::is_specialization_of<T, quoted_expression_t>::value;

    template<class T>
    constexpr bool
        is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, quoted_expression_t>::value>> =
            true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Public interface for syntax sugar for columns. Example: `where(c(&User::id) == 5)` or
     *  `storage.update(set(c(&User::name) = "Dua Lipa"));
     */
    template<class T>
    constexpr internal::quoted_expression_t<T> c(T value) {
        return {std::move(value)};
    }
}
