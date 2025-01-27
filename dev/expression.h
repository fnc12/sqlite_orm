#pragma once

#include <tuple>
#include <type_traits>  //  std::enable_if
#include <utility>  //  std::move, std::forward, std::declval
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "tags.h"
#include "operators.h"

namespace sqlite_orm {

    namespace internal {

        template<class L, class... Args>
        struct in_t;

        template<class L, class R>
        struct and_condition_t;

        template<class L, class R>
        struct or_condition_t;

        /**
         *  Result of c(...) function. Has operator= overloaded which returns assign_t
         */
        template<class T>
        struct expression_t {
            T value;

            template<class R>
            assign_t<T, R> operator=(R r) const {
                return {this->value, std::move(r)};
            }

            assign_t<T, nullptr_t> operator=(nullptr_t) const {
                return {this->value, nullptr};
            }
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
            assign_t<T, std::nullopt_t> operator=(std::nullopt_t) const {
                return {this->value, std::nullopt};
            }
#endif
            template<class... Args>
            in_t<T, Args...> in(Args... args) const {
                return {this->value, {std::forward<Args>(args)...}, false};
            }

            template<class... Args>
            in_t<T, Args...> not_in(Args... args) const {
                return {this->value, {std::forward<Args>(args)...}, true};
            }

            template<class R>
            and_condition_t<T, R> and_(R right) const {
                return {this->value, std::move(right)};
            }

            template<class R>
            or_condition_t<T, R> or_(R right) const {
                return {this->value, std::move(right)};
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_operator_argument_v<T, std::enable_if_t<polyfill::is_specialization_of<T, expression_t>::value>> = true;

        template<class T>
        constexpr T get_from_expression(T&& value) {
            return std::move(value);
        }

        template<class T>
        constexpr const T& get_from_expression(const T& value) {
            return value;
        }

        template<class T>
        constexpr T get_from_expression(expression_t<T>&& expression) {
            return std::move(expression.value);
        }

        template<class T>
        constexpr const T& get_from_expression(const expression_t<T>& expression) {
            return expression.value;
        }

        template<class T>
        using unwrap_expression_t = decltype(get_from_expression(std::declval<T>()));
    }

    /**
     *  Public interface for syntax sugar for columns. Example: `where(c(&User::id) == 5)` or
     * `storage.update(set(c(&User::name) = "Dua Lipa"));
     */
    template<class T>
    constexpr internal::expression_t<T> c(T value) {
        return {std::move(value)};
    }
}
