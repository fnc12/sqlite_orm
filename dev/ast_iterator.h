#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_invocable
#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/type_traits.h"
#include "tuple_helper/tuple_iteration.h"
#include "vocabulary/node_traits.h"
#include "ast/alias.h"
#include "prepared_statement.h"

namespace sqlite_orm::internal {
    /**
     *  ast_iterator accepts any expression and a callable object
     *  which will be called for any node of provided expression.
     *  E.g. if we pass `where(is_equal(5, max(&User::id, 10))` then
     *  callable object will be called with 5, &User::id and 10.
     *  ast_iterator is used in finding literals to be bound to
     *  a statement, and to collect table names.
     *  
     *  Note that not all leaves of the expression tree are visited:
     *  Column expressions can be more complex, but are passed as a whole to the callable.
     *  Examples are `column_pointer<>` and `alias_column_t<>`.
     *  
     *  To use `ast_iterator` call `iterate_ast(object, callable);`
     *  
     *  `T` is an ast element, e.g. where_t
     */
    template<class T, class SFINAE = void>
    struct ast_iterator {
        using node_type = T;

        /**
         *  L is a callable type. Mostly is a templated lambda
         */
        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& leaf, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            lambda(leaf);
        }
    };

    /**
     *  Simplified API.
     *  
     *  @param lambda Callable invoked with each leaf expression.
     *  The callable can opt in to be invoked for a node expression.
     */
    template<class T, class L>
    void iterate_ast(const T& t, L&& lambda) {
        ast_iterator<T> iterator;

        // possibly invoke lambda with node itself
        if constexpr (std::is_invocable<L, std::true_type, const T&>::value) {
            lambda(std::true_type{}, t);
        }

        iterator(t, lambda);
    }

    template<class T>
    struct ast_iterator<T, match_if<is_as_optional, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<std::reference_wrapper<T>, void> {
        using node_type = std::reference_wrapper<T>;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& expression, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(expression.get(), lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<std::disjunction<is_match<T>, is_match_with_table<T>>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.argument, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_any_group_by<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.args, lambda);
            if constexpr (/*is_group_by_with_having*/ polyfill::is_detected_v<expression_type_t, node_type>) {
                iterate_ast(node.expression, lambda);
            }
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_excluded, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& expression, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(expression.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_upsert_clause, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& expression, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(expression.actions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_table_valued_expression, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& expression, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(expression.table_values, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_from2, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& from, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(from.table_expressions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_where<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& expression, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(expression.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<std::disjunction<is_binary_condition<T>, is_binary_operator<T>>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.lhs, lambda);
            iterate_ast(node.rhs, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_equal_with_table, T>> {
        using node_type = T;

        template<class C>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, C& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.rhs, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<std::disjunction<is_columns<T>, is_struct<T>>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& cols, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(cols.columns, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_any_in, T>> {
        using node_type = T;

        template<class C>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& in, C& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(in.left, lambda);
            iterate_ast(in.argument, lambda);
        }
    };

    template<class T>
    struct ast_iterator<std::vector<T>, void> {
        using node_type = std::vector<T>;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& vec, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            for (auto& i: vec) {
                iterate_ast(i, lambda);
            }
        }
    };

    template<>
    struct ast_iterator<std::vector<char>, void> {
        using node_type = std::vector<char>;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& vec, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            lambda(vec);
        }
    };

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    template<class CTE>
    struct ast_iterator<CTE, match_if<is_cte_binding, CTE>> {
        using node_type = CTE;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& c, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(c.subselect, lambda);
        }
    };

    template<class With>
    struct ast_iterator<With, match_if<is_with_clause, With>> {
        using node_type = With;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& c, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(c.cte, lambda);
            iterate_ast(c.expression, lambda);
        }
    };
#endif

    template<class T>
    struct ast_iterator<T, match_if<is_compound_operator, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& c, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(c.compound, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_into, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& /*node*/, L& /*lambda*/) SQLITE_ORM_OR_CONST_CALLOP {
            //..
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_insert_raw_v<T> || is_replace_raw_v<T>>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.args, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_select<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& sel, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(sel.col, lambda);
            iterate_ast(sel.conditions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_any_get_all, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& get, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(get.conditions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_update_all_v<T>>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.set, lambda);
            iterate_ast(node.conditions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_remove_all_v<T>>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.conditions, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_set, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.assigns, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_dynamic_set, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.entries, lambda);
        }
    };

    template<class... Args>
    struct ast_iterator<std::tuple<Args...>, void> {
        using node_type = std::tuple<Args...>;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_tuple(node, [&lambda](auto& v) {
                iterate_ast(v, lambda);
            });
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_cast, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& c, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(c.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_exists, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_like, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& lk, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(lk._arg, lambda);
            iterate_ast(lk._pattern, lambda);
            lk._escape.apply([&lambda](auto& value) {
                iterate_ast(value, lambda);
            });
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_glob, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& lk, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(lk.arg, lambda);
            iterate_ast(lk.pattern, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_between, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& b, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(b.expression, lambda);
            iterate_ast(b.lower, lambda);
            iterate_ast(b.upper, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<std::disjunction<is_collate<T>, is_named_collate<T>>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& col, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(col.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_negated_condition, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& neg, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(neg.c, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<std::disjunction<is_is_null<T>, is_is_not_null<T>>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.argument, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_app_function_call, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.callArgs, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_builtin_function_call, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.args, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_filtered_aggregate_function, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.function, lambda);
            iterate_ast(node.where, lambda);
        }
    };

    //  a join, and its constraint: ON or USING, or the implicit one of CROSS JOIN and NATURAL JOIN
    template<class Join>
    struct ast_iterator<Join, match_if<is_any_join, Join>> {
        using node_type = Join;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& join, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(join.constraint, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_on, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& on, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(on.arg, lambda);
        }
    };

    // note: not strictly necessary as there's no binding support for USING;
    // we provide it nevertheless, in line with on_t.
    template<class T>
    struct ast_iterator<T, match_if<is_using, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& o, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(o.column, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_case_expression, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& c, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            c.case_expression.apply([&lambda](auto& c_) {
                iterate_ast(c_, lambda);
            });
            iterate_tuple(c.args, [&lambda](auto& pair) {
                iterate_ast(pair.first, lambda);
                iterate_ast(pair.second, lambda);
            });
            c.else_expression.apply([&lambda](auto& el) {
                iterate_ast(el, lambda);
            });
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_as_node<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_limit<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            // ...
            if constexpr (!node_type::has_offset_v) {
                iterate_ast(node.limit, lambda);
            }
            // ...
            else if constexpr (!node_type::offset_is_implicit_v) {
                iterate_ast(node.limit, lambda);
                node.offset.apply([&lambda](auto& value) {
                    iterate_ast(value, lambda);
                });
            }
            // ...
            else {
                node.offset.apply([&lambda](auto& value) {
                    iterate_ast(value, lambda);
                });
                iterate_ast(node.limit, lambda);
            }
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_rowset_deduplicator<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& a, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(a.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_unary_operator, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& a, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(a.argument, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_values, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.tuple, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_dynamic_values, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.vector, lambda);
        }
    };

    /**
     *  Column alias or literal: skipped
     */
    template<class T>
    struct ast_iterator<
        T,
        std::enable_if_t<
            std::disjunction<is_alias_holder<T>, is_literal<T>, is_column_alias<T>, is_implicit_join_constraint<T>>::
                value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& /*node*/, L& /*lambda*/) SQLITE_ORM_OR_CONST_CALLOP {}
    };

    template<class T>
    struct ast_iterator<T, match_if<is_order_by, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node._expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, match_if<is_multi_order_by, T>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.args, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_preceding<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_following<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.expression, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_frame_spec<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.start, lambda);
            iterate_ast(node.end, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_partition_by<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.arguments, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_over<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.function, lambda);
            iterate_ast(node.arguments, lambda);
        }
    };

    template<class T>
    struct ast_iterator<T, std::enable_if_t<is_window_defn<T>::value>> {
        using node_type = T;

        template<class L>
        SQLITE_ORM_STATIC_CALLOP void operator()(const node_type& node, L& lambda) SQLITE_ORM_OR_CONST_CALLOP {
            iterate_ast(node.arguments, lambda);
        }
    };
}
