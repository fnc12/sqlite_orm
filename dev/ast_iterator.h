#pragma once

#include <vector>  //  std::vector
#include <functional>  //  std::reference_wrapper

#include "conditions.h"
#include "select_constraints.h"
#include "operators.h"
#include "tuple_helper/tuple_helper.h"
#include "core_functions.h"
#include "prepared_statement.h"
#include "values.h"
#include "function.h"
#include "ast/excluded.h"
#include "ast/upsert_clause.h"
#include "ast/where.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  ast_iterator accepts any expression and a callable object
         *  which will be called for any node of provided expression.
         *  E.g. if we pass `where(is_equal(5, max(&User::id, 10))` then
         *  callable object will be called with 5, &User::id and 10.
         *  ast_iterator is used mostly in finding literals to be bound to
         *  a statement. To use it just call `iterate_ast(object, callable);`
         *  T is an ast element. E.g. where_t
         */
        template<class T, class SFINAE = void>
        struct ast_iterator {
            using node_type = T;

            /**
             *  L is a callable type. Mostly is a templated lambda
             */
            template<class L>
            void operator()(const T& t, const L& l) const {
                l(t);
            }
        };

        /**
         *  Simplified API
         */
        template<class T, class L>
        void iterate_ast(const T& t, const L& l) {
            ast_iterator<T> iterator;
            iterator(t, l);
        }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct ast_iterator<as_optional_t<T>, void> {
            using node_type = as_optional_t<T>;

            template<class L>
            void operator()(const node_type& node, const L& lambda) const {
                iterate_ast(node.value, lambda);
            }
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class T>
        struct ast_iterator<std::reference_wrapper<T>, void> {
            using node_type = std::reference_wrapper<T>;

            template<class L>
            void operator()(const node_type& r, const L& lambda) const {
                iterate_ast(r.get(), lambda);
            }
        };

        template<class T>
        struct ast_iterator<excluded_t<T>, void> {
            using node_type = excluded_t<T>;

            template<class L>
            void operator()(const node_type& expression, const L& lambda) const {
                iterate_ast(expression.expression, lambda);
            }
        };

        template<class... TargetArgs, class... ActionsArgs>
        struct ast_iterator<upsert_clause<std::tuple<TargetArgs...>, std::tuple<ActionsArgs...>>, void> {
            using node_type = upsert_clause<std::tuple<TargetArgs...>, std::tuple<ActionsArgs...>>;

            template<class L>
            void operator()(const node_type& expression, const L& lambda) const {
                iterate_ast(expression.actions, lambda);
            }
        };

        template<class C>
        struct ast_iterator<where_t<C>, void> {
            using node_type = where_t<C>;

            template<class L>
            void operator()(const node_type& expression, const L& lambda) const {
                iterate_ast(expression.expression, lambda);
            }
        };

        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, binary_condition>::value>::type> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& binaryCondition, const L& l) const {
                iterate_ast(binaryCondition.l, l);
                iterate_ast(binaryCondition.r, l);
            }
        };

        template<class L, class R, class... Ds>
        struct ast_iterator<binary_operator<L, R, Ds...>, void> {
            using node_type = binary_operator<L, R, Ds...>;

            template<class C>
            void operator()(const node_type& binaryOperator, const C& l) const {
                iterate_ast(binaryOperator.lhs, l);
                iterate_ast(binaryOperator.rhs, l);
            }
        };

        template<class... Args>
        struct ast_iterator<columns_t<Args...>, void> {
            using node_type = columns_t<Args...>;

            template<class L>
            void operator()(const node_type& cols, const L& l) const {
                iterate_ast(cols.columns, l);
            }
        };

        template<class L, class A>
        struct ast_iterator<dynamic_in_t<L, A>, void> {
            using node_type = dynamic_in_t<L, A>;

            template<class C>
            void operator()(const node_type& in, const C& l) const {
                iterate_ast(in.left, l);
                iterate_ast(in.argument, l);
            }
        };

        template<class L, class... Args>
        struct ast_iterator<in_t<L, Args...>, void> {
            using node_type = in_t<L, Args...>;

            template<class C>
            void operator()(const node_type& in, const C& l) const {
                iterate_ast(in.left, l);
                iterate_ast(in.argument, l);
            }
        };

        template<class T>
        struct ast_iterator<std::vector<T>, void> {
            using node_type = std::vector<T>;

            template<class L>
            void operator()(const node_type& vec, const L& l) const {
                for(auto& i: vec) {
                    iterate_ast(i, l);
                }
            }
        };

        template<>
        struct ast_iterator<std::vector<char>, void> {
            using node_type = std::vector<char>;

            template<class L>
            void operator()(const node_type& vec, const L& l) const {
                l(vec);
            }
        };

        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, compound_operator>::value>::type> {
            using node_type = T;

            template<class L>
            void operator()(const node_type& c, const L& l) const {
                iterate_ast(c.left, l);
                iterate_ast(c.right, l);
            }
        };

        template<class T>
        struct ast_iterator<into_t<T>, void> {
            using node_type = into_t<T>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                //..
            }
        };

        template<class... Args>
        struct ast_iterator<insert_raw_t<Args...>, void> {
            using node_type = insert_raw_t<Args...>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                iterate_ast(node.args, l);
            }
        };

        template<class... Args>
        struct ast_iterator<replace_raw_t<Args...>, void> {
            using node_type = replace_raw_t<Args...>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                iterate_ast(node.args, l);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<select_t<T, Args...>, void> {
            using node_type = select_t<T, Args...>;

            template<class L>
            void operator()(const node_type& sel, const L& l) const {
                iterate_ast(sel.col, l);
                iterate_ast(sel.conditions, l);
            }
        };

        template<class T, class R, class... Args>
        struct ast_iterator<get_all_t<T, R, Args...>, void> {
            using node_type = get_all_t<T, R, Args...>;

            template<class L>
            void operator()(const node_type& get, const L& l) const {
                iterate_ast(get.conditions, l);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<get_all_pointer_t<T, Args...>, void> {
            using node_type = get_all_pointer_t<T, Args...>;

            template<class L>
            void operator()(const node_type& get, const L& l) const {
                iterate_ast(get.conditions, l);
            }
        };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T, class... Args>
        struct ast_iterator<get_all_optional_t<T, Args...>, void> {
            using node_type = get_all_optional_t<T, Args...>;

            template<class L>
            void operator()(const node_type& get, const L& l) const {
                iterate_ast(get.conditions, l);
            }
        };
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

        template<class... Args, class... Wargs>
        struct ast_iterator<update_all_t<set_t<Args...>, Wargs...>, void> {
            using node_type = update_all_t<set_t<Args...>, Wargs...>;

            template<class L>
            void operator()(const node_type& u, const L& l) const {
                iterate_ast(u.set, l);
                iterate_ast(u.conditions, l);
            }
        };

        template<class T, class... Args>
        struct ast_iterator<remove_all_t<T, Args...>, void> {
            using node_type = remove_all_t<T, Args...>;

            template<class L>
            void operator()(const node_type& r, const L& l) const {
                iterate_ast(r.conditions, l);
            }
        };

        template<class... Args>
        struct ast_iterator<set_t<Args...>, void> {
            using node_type = set_t<Args...>;

            template<class L>
            void operator()(const node_type& s, const L& l) const {
                iterate_ast(s.assigns, l);
            }
        };

        template<class... Args>
        struct ast_iterator<std::tuple<Args...>, void> {
            using node_type = std::tuple<Args...>;

            template<class L>
            void operator()(const node_type& tuple, const L& l) const {
                iterate_tuple(tuple, [&l](auto& v) {
                    iterate_ast(v, l);
                });
            }
        };

        template<class T>
        struct ast_iterator<having_t<T>, void> {
            using node_type = having_t<T>;

            template<class L>
            void operator()(const node_type& hav, const L& l) const {
                iterate_ast(hav.t, l);
            }
        };

        template<class T, class E>
        struct ast_iterator<cast_t<T, E>, void> {
            using node_type = cast_t<T, E>;

            template<class L>
            void operator()(const node_type& c, const L& l) const {
                iterate_ast(c.expression, l);
            }
        };

        template<class T>
        struct ast_iterator<exists_t<T>, void> {
            using node_type = exists_t<T>;

            template<class L>
            void operator()(const node_type& e, const L& l) const {
                iterate_ast(e.t, l);
            }
        };

        template<class A, class T, class E>
        struct ast_iterator<like_t<A, T, E>, void> {
            using node_type = like_t<A, T, E>;

            template<class L>
            void operator()(const node_type& lk, const L& l) const {
                iterate_ast(lk.arg, l);
                iterate_ast(lk.pattern, l);
                lk.arg3.apply([&l](auto& value) {
                    iterate_ast(value, l);
                });
            }
        };

        template<class A, class T>
        struct ast_iterator<glob_t<A, T>, void> {
            using node_type = glob_t<A, T>;

            template<class L>
            void operator()(const node_type& lk, const L& l) const {
                iterate_ast(lk.arg, l);
                iterate_ast(lk.pattern, l);
            }
        };

        template<class A, class T>
        struct ast_iterator<between_t<A, T>, void> {
            using node_type = between_t<A, T>;

            template<class L>
            void operator()(const node_type& b, const L& l) const {
                iterate_ast(b.expr, l);
                iterate_ast(b.b1, l);
                iterate_ast(b.b2, l);
            }
        };

        template<class T>
        struct ast_iterator<named_collate<T>, void> {
            using node_type = named_collate<T>;

            template<class L>
            void operator()(const node_type& col, const L& l) const {
                iterate_ast(col.expr, l);
            }
        };

        template<class C>
        struct ast_iterator<negated_condition_t<C>, void> {
            using node_type = negated_condition_t<C>;

            template<class L>
            void operator()(const node_type& neg, const L& l) const {
                iterate_ast(neg.c, l);
            }
        };

        template<class T>
        struct ast_iterator<is_null_t<T>, void> {
            using node_type = is_null_t<T>;

            template<class L>
            void operator()(const node_type& i, const L& l) const {
                iterate_ast(i.t, l);
            }
        };

        template<class T>
        struct ast_iterator<is_not_null_t<T>, void> {
            using node_type = is_not_null_t<T>;

            template<class L>
            void operator()(const node_type& i, const L& l) const {
                iterate_ast(i.t, l);
            }
        };

        template<class F, class... Args>
        struct ast_iterator<function_call<F, Args...>, void> {
            using node_type = function_call<F, Args...>;

            template<class L>
            void operator()(const node_type& f, const L& l) const {
                iterate_ast(f.args, l);
            }
        };

        template<class R, class S, class... Args>
        struct ast_iterator<built_in_function_t<R, S, Args...>, void> {
            using node_type = built_in_function_t<R, S, Args...>;

            template<class L>
            void operator()(const node_type& f, const L& l) const {
                iterate_ast(f.args, l);
            }
        };

        template<class T, class O>
        struct ast_iterator<left_join_t<T, O>, void> {
            using node_type = left_join_t<T, O>;

            template<class L>
            void operator()(const node_type& j, const L& l) const {
                iterate_ast(j.constraint, l);
            }
        };

        template<class T>
        struct ast_iterator<on_t<T>, void> {
            using node_type = on_t<T>;

            template<class L>
            void operator()(const node_type& o, const L& l) const {
                iterate_ast(o.arg, l);
            }
        };

        template<class T, class O>
        struct ast_iterator<join_t<T, O>, void> {
            using node_type = join_t<T, O>;

            template<class L>
            void operator()(const node_type& j, const L& l) const {
                iterate_ast(j.constraint, l);
            }
        };

        template<class T, class O>
        struct ast_iterator<left_outer_join_t<T, O>, void> {
            using node_type = left_outer_join_t<T, O>;

            template<class L>
            void operator()(const node_type& j, const L& l) const {
                iterate_ast(j.constraint, l);
            }
        };

        template<class T, class O>
        struct ast_iterator<inner_join_t<T, O>, void> {
            using node_type = inner_join_t<T, O>;

            template<class L>
            void operator()(const node_type& j, const L& l) const {
                iterate_ast(j.constraint, l);
            }
        };

        template<class R, class T, class E, class... Args>
        struct ast_iterator<simple_case_t<R, T, E, Args...>, void> {
            using node_type = simple_case_t<R, T, E, Args...>;

            template<class L>
            void operator()(const node_type& c, const L& l) const {
                c.case_expression.apply([&l](auto& c_) {
                    iterate_ast(c_, l);
                });
                iterate_tuple(c.args, [&l](auto& pair) {
                    iterate_ast(pair.first, l);
                    iterate_ast(pair.second, l);
                });
                c.else_expression.apply([&l](auto& el) {
                    iterate_ast(el, l);
                });
            }
        };

        template<class T, class E>
        struct ast_iterator<as_t<T, E>, void> {
            using node_type = as_t<T, E>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.expression, l);
            }
        };

        template<class T, bool OI>
        struct ast_iterator<limit_t<T, false, OI, void>, void> {
            using node_type = limit_t<T, false, OI, void>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.lim, l);
            }
        };

        template<class T, class O>
        struct ast_iterator<limit_t<T, true, false, O>, void> {
            using node_type = limit_t<T, true, false, O>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.lim, l);
                a.off.apply([&l](auto& value) {
                    iterate_ast(value, l);
                });
            }
        };

        template<class T, class O>
        struct ast_iterator<limit_t<T, true, true, O>, void> {
            using node_type = limit_t<T, true, true, O>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                a.off.apply([&l](auto& value) {
                    iterate_ast(value, l);
                });
                iterate_ast(a.lim, l);
            }
        };

        template<class T>
        struct ast_iterator<distinct_t<T>, void> {
            using node_type = distinct_t<T>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.value, l);
            }
        };

        template<class T>
        struct ast_iterator<all_t<T>, void> {
            using node_type = all_t<T>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.value, l);
            }
        };

        template<class T>
        struct ast_iterator<bitwise_not_t<T>, void> {
            using node_type = bitwise_not_t<T>;

            template<class L>
            void operator()(const node_type& a, const L& l) const {
                iterate_ast(a.argument, l);
            }
        };

        template<class... Args>
        struct ast_iterator<values_t<Args...>, void> {
            using node_type = values_t<Args...>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                iterate_ast(node.tuple, l);
            }
        };

        template<class T>
        struct ast_iterator<dynamic_values_t<T>, void> {
            using node_type = dynamic_values_t<T>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                iterate_ast(node.vector, l);
            }
        };

        template<class T>
        struct ast_iterator<collate_t<T>, void> {
            using node_type = collate_t<T>;

            template<class L>
            void operator()(const node_type& node, const L& l) const {
                iterate_ast(node.expr, l);
            }
        };

    }
}
