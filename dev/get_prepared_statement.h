#pragma once

#include <type_traits>  //  std::is_same, std::decay, std::remove_reference

#include "prepared_statement.h"
#include "ast_iterator.h"
#include "static_magic.h"

namespace sqlite_orm {

    template<int N, class T>
    const auto &get(const internal::prepared_statement_t<T> &statement) {
        using statement_type = typename std::decay<decltype(statement)>::type;
        using expression_type = typename statement_type::expression_type;
        using node_tuple = typename internal::node_tuple<expression_type>::type;
        using bind_tuple = typename internal::bindable_filter<node_tuple>::type;
        using result_tupe = typename std::tuple_element<N, bind_tuple>::type;
        const result_tupe *result = nullptr;
        auto index = -1;
        internal::iterate_ast(statement.t, [&result, &index](auto &node) {
            using node_type = typename std::decay<decltype(node)>::type;
            if(internal::is_bindable<node_type>::value) {
                ++index;
            }
            if(index == N) {
                internal::static_if<std::is_same<result_tupe, node_type>{}>([](auto &result, auto &node) {
                    result = const_cast<typename std::remove_reference<decltype(result)>::type>(&node);
                })(result, node);
            }
        });
        return *result;
    }

    template<int N, class T>
    auto &get(internal::prepared_statement_t<T> &statement) {
        using statement_type = typename std::decay<decltype(statement)>::type;
        using expression_type = typename statement_type::expression_type;
        using node_tuple = typename internal::node_tuple<expression_type>::type;
        using bind_tuple = typename internal::bindable_filter<node_tuple>::type;
        using result_tupe = typename std::tuple_element<N, bind_tuple>::type;
        result_tupe *result = nullptr;
        auto index = -1;
        internal::iterate_ast(statement.t, [&result, &index](auto &node) {
            using node_type = typename std::decay<decltype(node)>::type;
            if(internal::is_bindable<node_type>::value) {
                ++index;
            }
            if(index == N) {
                internal::static_if<std::is_same<result_tupe, node_type>{}>([](auto &result, auto &node) {
                    result = const_cast<typename std::remove_reference<decltype(result)>::type>(&node);
                })(result, node);
            }
        });
        return *result;
    }
}
