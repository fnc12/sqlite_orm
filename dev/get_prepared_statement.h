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

    template<
        int N,
        class E,
        std::enable_if_t<std::disjunction_v<internal::is_insert_range<E>, internal::is_replace_range<E>>, bool> = true>
    auto& get(internal::prepared_statement_t<E>& statement) {
        return std::get<N>(statement.expression.range);
    }

    template<
        int N,
        class E,
        std::enable_if_t<std::disjunction_v<internal::is_insert_range<E>, internal::is_replace_range<E>>, bool> = true>
    const auto& get(const internal::prepared_statement_t<E>& statement) {
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

    template<int N, class E, internal::satisfies<internal::is_remove, E> = true>
    auto& get(internal::prepared_statement_t<E>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    template<int N, class E, internal::satisfies<internal::is_remove, E> = true>
    const auto& get(const internal::prepared_statement_t<E>& statement) {
        return internal::forward_lvalue_ref(std::get<N>(statement.expression.ids));
    }

    //  insert, insert explicit, replace, update: the object is the only bound value
    template<int N,
             class E,
             std::enable_if_t<
                 std::conjunction_v<internal::is_object_dml_expression<E>, std::negation<internal::is_remove<E>>>,
                 bool> = true>
    auto& get(internal::prepared_statement_t<E>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert, replace and update statements");
        return internal::access_dml_object(statement);
    }

    template<int N,
             class E,
             std::enable_if_t<
                 std::conjunction_v<internal::is_object_dml_expression<E>, std::negation<internal::is_remove<E>>>,
                 bool> = true>
    const auto& get(const internal::prepared_statement_t<E>& statement) {
        static_assert(N == 0, "get<> works only with 0 argument for insert, replace and update statements");
        return internal::access_dml_object(statement);
    }

    //  note: the statements above bind their values in a way of their own, hence the exclusion
    template<int N,
             class T,
             std::enable_if_t<std::negation_v<std::disjunction<internal::is_object_dml_expression<T>,
                                                               internal::is_insert_range<T>,
                                                               internal::is_replace_range<T>>>,
                              bool> = true>
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

    template<int N,
             class T,
             std::enable_if_t<std::negation_v<std::disjunction<internal::is_object_dml_expression<T>,
                                                               internal::is_insert_range<T>,
                                                               internal::is_replace_range<T>>>,
                              bool> = true>
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
