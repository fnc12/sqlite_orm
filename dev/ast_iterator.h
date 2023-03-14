#pragma once

#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper

#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "conditions.h"
#include "alias.h"
#include "select_constraints.h"
#include "operators.h"
#include "core_functions.h"
#include "prepared_statement.h"
#include "values.h"
#include "function.h"
#include "ast/excluded.h"
#include "ast/upsert_clause.h"
#include "ast/where.h"
#include "ast/into.h"
#include "ast/group_by.h"
#include "ast/exists.h"
#include "ast/set.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  ast_iterator accepts any expression and a callable object
         *  which will be called for any node of provided expression.
         *  E.g. if we pass `where(is_equal(5, max(&User::id, 10))` then
         *  callable object will be called with 5, &User::id and 10.
         *  ast_iterator is used in finding literals to be bound to
         *  a statement, and to collect table names.
         *  
         *  Note that not all leaves of the expression tree are always visited:
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
            void operator()(const T& t, L& lambda) const {
                lambda(t);
            }
        };

        /**
         *  Simplified API
         */
        template<class T, class L>
        void iterate_ast(const T& t, L&& lambda) {
            ast_iterator<T> iterator;
            iterator(t, lambda);
        }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct ast_iterator<as_optional_t<T>, void> {
            using node_type = as_optional_t<T>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.value, lambda);
            }
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class T>
        struct ast_iterator<std::reference_wrapper<T>, void> {
            using node_type = std::reference_wrapper<T>;

            template<class L>
            void operator()(const node_type& expression, L& lambda) const {
                iterate_ast(expression.get(), lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<group_by_t<Args...>, void> {
            using node_type = group_by_t<Args...>;

            template<class L>
            void operator()(const node_type& expression, L& lambda) const {
                iterate_ast(expression.args, lambda);
            }
        };

        template<class T>
        struct ast_iterator<excluded_t<T>, void> {
            using node_type = excluded_t<T>;

            template<class L>
            void operator()(const node_type& expression, L& lambda) const {
                iterate_ast(expression.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<T, match_if<is_upsert_clause, T>> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& expression, L& lambda) const {
                iterate_ast(expression.actions, lambda);
            }
        };

        template<class C>
        struct ast_iterator<where_t<C>, void> {
            using node_type = where_t<C>;

            template<class L>
            void operator()(const node_type& expression, L& lambda) const {
                iterate_ast(expression.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<T, match_if<is_binary_condition, T>> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& binaryCondition, L& lambda) const {
                iterate_ast(binaryCondition.l, lambda);
                iterate_ast(binaryCondition.r, lambda);
            }
        };

        template<class L, class R, class... Ds>
        struct ast_iterator<binary_operator<L, R, Ds...>, void> {
            using node_type = binary_operator<L, R, Ds...>;

            template<class C>
            void operator()(const node_type& binaryOperator, C& lambda) const {
                iterate_ast(binaryOperator.lhs, lambda);
                iterate_ast(binaryOperator.rhs, lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<columns_t<Args...>, void> {
            using node_type = columns_t<Args...>;

            template<class L>
            void operator()(const node_type& cols, L& lambda) const {
                iterate_ast(cols.columns, lambda);
            }
        };

        template<class L, class A>
        struct ast_iterator<dynamic_in_t<L, A>, void> {
            using node_type = dynamic_in_t<L, A>;

            template<class C>
            void operator()(const node_type& in, C& lambda) const {
                iterate_ast(in.left, lambda);
                iterate_ast(in.argument, lambda);
            }
        };

        template<class L, class... Args>
        struct ast_iterator<in_t<L, Args...>, void> {
            using node_type = in_t<L, Args...>;

            template<class C>
            void operator()(const node_type& in, C& lambda) const {
                iterate_ast(in.left, lambda);
                iterate_ast(in.argument, lambda);
            }
        };

        template<class T>
        struct ast_iterator<std::vector<T>, void> {
            using node_type = std::vector<T>;

            template<class L>
            void operator()(const node_type& vec, L& lambda) const {
                for(auto& i: vec) {
                    iterate_ast(i, lambda);
                }
            }
        };

        template<>
        struct ast_iterator<std::vector<char>, void> {
            using node_type = std::vector<char>;

            template<class L>
            void operator()(const node_type& vec, L& lambda) const {
                lambda(vec);
            }
        };

        template<class T>
        struct ast_iterator<T, match_if<is_compound_operator, T>> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& c, L& lambda) const {
                iterate_ast(c.left, lambda);
                iterate_ast(c.right, lambda);
            }
        };

        template<class T>
        struct ast_iterator<into_t<T>, void> {
            using node_type = into_t<T>;

            template<class L>
            void operator()(const node_type& /*node*/, L& /*lambda*/) const {
                //..
            }
        };

        template<class... Args>
        struct ast_iterator<insert_raw_t<Args...>, void> {
            using node_type = insert_raw_t<Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.args, lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<replace_raw_t<Args...>, void> {
            using node_type = replace_raw_t<Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.args, lambda);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<select_t<T, Args...>, void> {
            using node_type = select_t<T, Args...>;

            template<class L>
            void operator()(const node_type& sel, L& lambda) const {
                iterate_ast(sel.col, lambda);
                iterate_ast(sel.conditions, lambda);
            }
        };

        template<class T, class R, class... Args>
        struct ast_iterator<get_all_t<T, R, Args...>, void> {
            using node_type = get_all_t<T, R, Args...>;

            template<class L>
            void operator()(const node_type& get, L& lambda) const {
                iterate_ast(get.conditions, lambda);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<get_all_pointer_t<T, Args...>, void> {
            using node_type = get_all_pointer_t<T, Args...>;

            template<class L>
            void operator()(const node_type& get, L& lambda) const {
                iterate_ast(get.conditions, lambda);
            }
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct ast_iterator<get_all_optional_t<T, Args...>, void> {
            using node_type = get_all_optional_t<T, Args...>;

            template<class L>
            void operator()(const node_type& get, L& lambda) const {
                iterate_ast(get.conditions, lambda);
            }
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class S, class... Wargs>
        struct ast_iterator<update_all_t<S, Wargs...>, void> {
            using node_type = update_all_t<S, Wargs...>;

            template<class L>
            void operator()(const node_type& u, L& lambda) const {
                iterate_ast(u.set, lambda);
                iterate_ast(u.conditions, lambda);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<remove_all_t<T, Args...>, void> {
            using node_type = remove_all_t<T, Args...>;

            template<class L>
            void operator()(const node_type& r, L& lambda) const {
                iterate_ast(r.conditions, lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<set_t<Args...>, void> {
            using node_type = set_t<Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.assigns, lambda);
            }
        };

        template<class S>
        struct ast_iterator<dynamic_set_t<S>, void> {
            using node_type = dynamic_set_t<S>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.entries, lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<std::tuple<Args...>, void> {
            using node_type = std::tuple<Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_tuple(node, [&lambda](auto& v) {
                    iterate_ast(v, lambda);
                });
            }
        };

        template<class T, class... Args>
        struct ast_iterator<group_by_with_having<T, Args...>, void> {
            using node_type = group_by_with_having<T, Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.args, lambda);
                iterate_ast(node.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<having_t<T>, void> {
            using node_type = having_t<T>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.expression, lambda);
            }
        };

        template<class T, class E>
        struct ast_iterator<cast_t<T, E>, void> {
            using node_type = cast_t<T, E>;

            template<class L>
            void operator()(const node_type& c, L& lambda) const {
                iterate_ast(c.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<exists_t<T>, void> {
            using node_type = exists_t<T>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.expression, lambda);
            }
        };

        template<class A, class T, class E>
        struct ast_iterator<like_t<A, T, E>, void> {
            using node_type = like_t<A, T, E>;

            template<class L>
            void operator()(const node_type& lk, L& lambda) const {
                iterate_ast(lk.arg, lambda);
                iterate_ast(lk.pattern, lambda);
                lk.arg3.apply([&lambda](auto& value) {
                    iterate_ast(value, lambda);
                });
            }
        };

        template<class A, class T>
        struct ast_iterator<glob_t<A, T>, void> {
            using node_type = glob_t<A, T>;

            template<class L>
            void operator()(const node_type& lk, L& lambda) const {
                iterate_ast(lk.arg, lambda);
                iterate_ast(lk.pattern, lambda);
            }
        };

        template<class A, class T>
        struct ast_iterator<between_t<A, T>, void> {
            using node_type = between_t<A, T>;

            template<class L>
            void operator()(const node_type& b, L& lambda) const {
                iterate_ast(b.expr, lambda);
                iterate_ast(b.b1, lambda);
                iterate_ast(b.b2, lambda);
            }
        };

        template<class T>
        struct ast_iterator<named_collate<T>, void> {
            using node_type = named_collate<T>;

            template<class L>
            void operator()(const node_type& col, L& lambda) const {
                iterate_ast(col.expr, lambda);
            }
        };

        template<class C>
        struct ast_iterator<negated_condition_t<C>, void> {
            using node_type = negated_condition_t<C>;

            template<class L>
            void operator()(const node_type& neg, L& lambda) const {
                iterate_ast(neg.c, lambda);
            }
        };

        template<class T>
        struct ast_iterator<is_null_t<T>, void> {
            using node_type = is_null_t<T>;

            template<class L>
            void operator()(const node_type& i, L& lambda) const {
                iterate_ast(i.t, lambda);
            }
        };

        template<class T>
        struct ast_iterator<is_not_null_t<T>, void> {
            using node_type = is_not_null_t<T>;

            template<class L>
            void operator()(const node_type& i, L& lambda) const {
                iterate_ast(i.t, lambda);
            }
        };

        template<class F, class... Args>
        struct ast_iterator<function_call<F, Args...>, void> {
            using node_type = function_call<F, Args...>;

            template<class L>
            void operator()(const node_type& f, L& lambda) const {
                iterate_ast(f.args, lambda);
            }
        };

        template<class R, class S, class... Args>
        struct ast_iterator<built_in_function_t<R, S, Args...>, void> {
            using node_type = built_in_function_t<R, S, Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.args, lambda);
            }
        };

        template<class R, class S, class... Args>
        struct ast_iterator<built_in_aggregate_function_t<R, S, Args...>, void> {
            using node_type = built_in_aggregate_function_t<R, S, Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.args, lambda);
            }
        };

        template<class F, class W>
        struct ast_iterator<filtered_aggregate_function<F, W>, void> {
            using node_type = filtered_aggregate_function<F, W>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.function, lambda);
                iterate_ast(node.where, lambda);
            }
        };

        template<class Join>
        struct ast_iterator<Join, match_if<is_constrained_join, Join>> {
            using node_type = Join;

            template<class L>
            void operator()(const node_type& join, L& lambda) const {
                iterate_ast(join.constraint, lambda);
            }
        };

        template<class T>
        struct ast_iterator<on_t<T>, void> {
            using node_type = on_t<T>;

            template<class L>
            void operator()(const node_type& on, L& lambda) const {
                iterate_ast(on.arg, lambda);
            }
        };

        // note: not strictly necessary as there's no binding support for USING;
        // we provide it nevertheless, in line with on_t.
        template<class T>
        struct ast_iterator<T, std::enable_if_t<polyfill::is_specialization_of_v<T, using_t>>> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& o, L& lambda) const {
                iterate_ast(o.column, lambda);
            }
        };

        template<class R, class T, class E, class... Args>
        struct ast_iterator<simple_case_t<R, T, E, Args...>, void> {
            using node_type = simple_case_t<R, T, E, Args...>;

            template<class L>
            void operator()(const node_type& c, L& lambda) const {
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

        template<class T, class E>
        struct ast_iterator<as_t<T, E>, void> {
            using node_type = as_t<T, E>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.expression, lambda);
            }
        };

        template<class T, bool OI>
        struct ast_iterator<limit_t<T, false, OI, void>, void> {
            using node_type = limit_t<T, false, OI, void>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.lim, lambda);
            }
        };

        template<class T, class O>
        struct ast_iterator<limit_t<T, true, false, O>, void> {
            using node_type = limit_t<T, true, false, O>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.lim, lambda);
                a.off.apply([&lambda](auto& value) {
                    iterate_ast(value, lambda);
                });
            }
        };

        template<class T, class O>
        struct ast_iterator<limit_t<T, true, true, O>, void> {
            using node_type = limit_t<T, true, true, O>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                a.off.apply([&lambda](auto& value) {
                    iterate_ast(value, lambda);
                });
                iterate_ast(a.lim, lambda);
            }
        };

        template<class T>
        struct ast_iterator<distinct_t<T>, void> {
            using node_type = distinct_t<T>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.value, lambda);
            }
        };

        template<class T>
        struct ast_iterator<all_t<T>, void> {
            using node_type = all_t<T>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.value, lambda);
            }
        };

        template<class T>
        struct ast_iterator<bitwise_not_t<T>, void> {
            using node_type = bitwise_not_t<T>;

            template<class L>
            void operator()(const node_type& a, L& lambda) const {
                iterate_ast(a.argument, lambda);
            }
        };

        template<class... Args>
        struct ast_iterator<values_t<Args...>, void> {
            using node_type = values_t<Args...>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.tuple, lambda);
            }
        };

        template<class T>
        struct ast_iterator<dynamic_values_t<T>, void> {
            using node_type = dynamic_values_t<T>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.vector, lambda);
            }
        };

        /**
         *  Column alias or literal: skipped
         */
        template<class T>
        struct ast_iterator<T,
                            std::enable_if_t<polyfill::disjunction_v<polyfill::is_specialization_of<T, alias_holder>,
                                                                     polyfill::is_specialization_of<T, literal_holder>,
                                                                     is_column_alias<T>>>> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& /*node*/, L& /*lambda*/) const {}
        };

        template<class E>
        struct ast_iterator<order_by_t<E>, void> {
            using node_type = order_by_t<E>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<collate_t<T>, void> {
            using node_type = collate_t<T>;

            template<class L>
            void operator()(const node_type& node, L& lambda) const {
                iterate_ast(node.expr, lambda);
            }
        };

    }
}
