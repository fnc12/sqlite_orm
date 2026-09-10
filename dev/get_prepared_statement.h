#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_same, std::remove_reference, std::remove_cvref
#include <tuple>  //  std::get
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/type_traits.h"
#include "vocabulary/node_traits.h"  // projections
#include "vocabulary/node_algorithms.h"  // access_dml_object
#include "ast/dml/insert.h"
#include "ast/dml/replace.h"
#include "ast/dml/update.h"
#include "ast/dml/remove.h"
#include "prepared_statement.h"
#include "ast_iterator.h"
#include "node_tuple.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    template<int N, class It, class L, class O>
    auto& get(internal::prepared_statement_t<internal::insert_range_t<It, L, O>>& statement) {
        return std::get<N>(statement.expression.range);
    }

    template<int N, class It, class L, class O>
    const auto& get(const internal::prepared_statement_t<internal::insert_range_t<It, L, O>>& statement) {
        return std::get<N>(statement.expression.range);
    }

    template<int N, class It, class L, class O>
    auto& get(internal::prepared_statement_t<internal::replace_range_t<It, L, O>>& statement) {
        return std::get<N>(statement.expression.range);
    }

    template<int N, class It, class L, class O>
    const auto& get(const internal::prepared_statement_t<internal::replace_range_t<It, L, O>>& statement) {
        return std::get<N>(statement.expression.range);
    }

    template<int N, class T, class... Ids>
    auto& get(internal::prepared_statement_t<internal::get_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    const auto& get(const internal::prepared_statement_t<internal::get_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    auto& get(internal::prepared_statement_t<internal::get_pointer_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    const auto& get(const internal::prepared_statement_t<internal::get_pointer_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    auto& get(internal::prepared_statement_t<internal::get_optional_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    const auto& get(const internal::prepared_statement_t<internal::get_optional_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    auto& get(internal::prepared_statement_t<internal::remove_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T, class... Ids>
    const auto& get(const internal::prepared_statement_t<internal::remove_t<T, Ids...>>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class T>
    auto& get(internal::prepared_statement_t<internal::update_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for update statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    const auto& get(const internal::prepared_statement_t<internal::update_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for update statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T, class... Cols>
    auto& get(internal::prepared_statement_t<internal::insert_explicit<T, Cols...>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T, class... Cols>
    const auto& get(const internal::prepared_statement_t<internal::insert_explicit<T, Cols...>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    auto& get(internal::prepared_statement_t<internal::replace_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for replace statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    const auto& get(const internal::prepared_statement_t<internal::replace_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for replace statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    auto& get(internal::prepared_statement_t<internal::insert_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    const auto& get(const internal::prepared_statement_t<internal::insert_t<T>>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert statement");
        return internal::access_dml_object(statement);
    }

    template<int N, class T>
    const auto& get(const internal::prepared_statement_t<T>& statement) {
        using namespace ::sqlite_orm::internal;
        using statement_type = polyfill::remove_cvref_t<decltype(statement)>;
        using expression_type = expression_type_t<statement_type>;
        using node_tuple = node_tuple_t<expression_type>;
        using bind_tuple = bindable_filter_t<node_tuple>;
        using result_type = std::tuple_element_t<static_cast<size_t>(N), bind_tuple>;
        const result_type* result = nullptr;
        iterate_ast(statement.expression, [&result, index = -1](auto& node) mutable {
            using node_type = polyfill::remove_cvref_t<decltype(node)>;
            if constexpr (is_bindable<node_type>::value) {
                ++index;
                if constexpr (std::is_same<result_type, node_type>::value) {
                    if (index == N) {
                        result = &node;
                    }
                }
            }
        });
        return forward_lvalue_ref(*result);
    }

    template<int N, class T>
    auto& get(internal::prepared_statement_t<T>& statement) {
        using namespace ::sqlite_orm::internal;
        using statement_type = std::remove_reference_t<decltype(statement)>;
        using expression_type = expression_type_t<statement_type>;
        using node_tuple = node_tuple_t<expression_type>;
        using bind_tuple = bindable_filter_t<node_tuple>;
        using result_type = std::tuple_element_t<static_cast<size_t>(N), bind_tuple>;
        result_type* result = nullptr;

        iterate_ast(statement.expression, [&result, index = -1](auto& node) mutable {
            using node_type = polyfill::remove_cvref_t<decltype(node)>;
            if constexpr (is_bindable<node_type>::value) {
                ++index;
                if constexpr (std::is_same<result_type, node_type>::value) {
                    if (index == N) {
                        result = const_cast<result_type*>(&node);
                    }
                }
            }
        });
        return forward_lvalue_ref(*result);
    }
}
