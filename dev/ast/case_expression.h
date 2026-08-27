#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if
#include <tuple>  //  std::tuple, std::get, std::tuple_cat, std::tuple_size, std::make_tuple
#include <utility>  //  std::move, std::pair
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../optional_container.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    template<class T>
    struct then_t {
        using expression_type = T;

        expression_type expression;
    };

    template<class R, class T, class E, class... Args>
    struct simple_case_t {
        using return_type = R;
        using case_expression_type = T;
        using args_type = std::tuple<Args...>;
        using else_expression_type = E;

        optional_container<case_expression_type> case_expression;
        args_type args;
        optional_container<else_expression_type> else_expression;
    };

    template<class T>
    constexpr bool is_case_expression_v = polyfill::is_specialization_of<T, simple_case_t>::value;

    template<class T>
    constexpr bool is_operator_argument_v<T, std::enable_if_t<is_case_expression_v<T>>> = true;

    /**
     *  T is a case expression type
     *  E is else type (void is ELSE is omitted)
     *  Args... is a pack of WHEN expressions
     */
    template<class R, class T, class E, class... Args>
    struct simple_case_builder {
        using return_type = R;
        using case_expression_type = T;
        using args_type = std::tuple<Args...>;
        using else_expression_type = E;

        optional_container<case_expression_type> case_expression;
        args_type args;
        optional_container<else_expression_type> else_expression;

        template<class W, class Th>
        simple_case_builder<R, T, E, Args..., std::pair<W, Th>> when(W w, then_t<Th> t) {
            using result_args_type = std::tuple<Args..., std::pair<W, Th>>;
            std::pair<W, Th> newPair{std::move(w), std::move(t.expression)};
            result_args_type result_args = std::tuple_cat(std::move(this->args), std::make_tuple(newPair));
            std::get<std::tuple_size<result_args_type>::value - 1>(result_args) = std::move(newPair);
            return {std::move(this->case_expression), std::move(result_args), std::move(this->else_expression)};
        }

        simple_case_t<R, T, E, Args...> end() {
            return {std::move(this->case_expression), std::move(args), std::move(this->else_expression)};
        }

        template<class El>
        simple_case_builder<R, T, El, Args...> else_(El el) {
            return {{std::move(this->case_expression)}, std::move(args), {std::move(el)}};
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T>
    internal::then_t<T> then(T t) {
        return {std::move(t)};
    }

    template<class R, class T>
    internal::simple_case_builder<R, T, void> case_(T t) {
        return {{std::move(t)}};
    }

    template<class R>
    internal::simple_case_builder<R, void, void> case_() {
        return {};
    }
}
