#pragma once

/** @file The nodes of a call of a built-in SQL function: scalar and aggregate function calls,
 *        an aggregate call with a FILTER clause, and COUNT(*).
 *        The functions themselves are in `core_functions.h`.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same
#include <tuple>  //  std::tuple, std::tuple_size
#include <string_view>  //  std::string_view
#include <utility>  //  std::move, std::forward
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/is_base_template_of.h"
#include "../functional/type_traits.h"  //  satisfies
#include "../tags.h"  //  arithmetic_t
#include "../vocabulary/node_traits.h"  //  is_where, expression_type_t
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/operand_traits_fwd.h"  // Included to specialize traits
#include "window.h"  //  over_t, validate_over_arguments

namespace sqlite_orm::internal {
    /**
     *  Base class for operator overloading
     *  R - return type
     *  S - class with operator std::string
     *  Args - function arguments types
     */
    template<class R, class S, class... Args>
    struct builtin_function_t : S, arithmetic_t {
        using return_type = R;
        using string_type = S;
        using args_tuple = std::tuple<Args...>;

        static constexpr size_t args_size = std::tuple_size<args_tuple>::value;

        args_tuple args;

        constexpr builtin_function_t(args_tuple&& args_) : args(std::move(args_)) {}
    };

    template<class T>
    constexpr bool is_builtin_function_v = is_base_template_of<builtin_function_t, T>::value;

    template<class F, class W>
    struct filtered_aggregate_function {
        using function_type = F;
        using where_expression = W;

        function_type function;
        where_expression where;

        template<class... OverArgs>
        over_t<filtered_aggregate_function, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    template<class T>
    constexpr bool is_filtered_aggregate_function_v = polyfill::is_specialization_of_v<T, filtered_aggregate_function>;

    template<class R, class S, class... Args>
    struct builtin_aggregate_function_t : builtin_function_t<R, S, Args...> {
        using super = builtin_function_t<R, S, Args...>;

        using super::super;

        template<class Wh, satisfies<is_where, Wh> = true>
        filtered_aggregate_function<builtin_aggregate_function_t, expression_type_t<Wh>> filter(Wh wh) {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        over_t<builtin_aggregate_function_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    struct count_string {
        std::string_view serialize() const {
            return "COUNT";
        }
    };

    /**
     *  T is use to specify type explicitly for queries like
     *  SELECT COUNT(*) FROM table_name;
     *  T can be omitted with void.
     */
    template<class T>
    struct count_asterisk_t : count_string {
        using type = T;

        template<class Wh, satisfies<is_where, Wh> = true>
        filtered_aggregate_function<count_asterisk_t<T>, expression_type_t<Wh>> filter(Wh wh) {
            return {*this, std::move(wh.expression)};
        }

        template<class... OverArgs>
        over_t<count_asterisk_t, OverArgs...> over(OverArgs... overArgs) {
            validate_over_arguments<OverArgs...>();
            return {*this, {std::forward<OverArgs>(overArgs)...}};
        }
    };

    /**
     *  The same thing as count<T>() but without T arg.
     *  Is used in cases like this:
     *    SELECT cust_code, cust_name, cust_city, grade
     *    FROM customer
     *    WHERE grade=2 AND EXISTS
     *        (SELECT COUNT(*)
     *        FROM customer
     *        WHERE grade=2
     *        GROUP BY grade
     *        HAVING COUNT(*)>2);
     *  `c++`
     *  auto rows =
     *      storage.select(columns(&Customer::code, &Customer::name, &Customer::city, &Customer::grade),
     *          where(is_equal(&Customer::grade, 2)
     *              and exists(select(count<Customer>(),
     *                  where(is_equal(&Customer::grade, 2)),
     *          group_by(&Customer::grade),
     *          having(greater_than(count(), 2))))));
     */
    struct count_asterisk_without_type : count_string {
        using type = void;
    };

    template<class T>
    constexpr bool is_count_asterisk_v =
        polyfill::is_specialization_of_v<T, count_asterisk_t> || std::is_same_v<T, count_asterisk_without_type>;

    template<class T>
    constexpr bool is_operator_argument_v<T, std::enable_if_t<is_count_asterisk_v<T>>> = true;
}
