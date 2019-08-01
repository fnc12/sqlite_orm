#pragma once

#include <vector>   //  std::vector

#include "conditions.h"
#include "select_constraints.h"
#include "operators.h"
#include "tuple_helper.h"
#include "core_functions.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  ast_iterator accepts an any expression and a callable object
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
             *  L is a callable type. Mostly is templated lambda
             */
            template<class L>
            void operator()(const T &t, const L &l) const {
                l(t);
            }
        };
        
        /**
         *  Simplified API
         */
        template<class T, class L>
        void iterate_ast(const T &t, const L &l) {
            ast_iterator<T> iterator;
            iterator(t, l);
        }
        
        template<class C>
        struct ast_iterator<conditions::where_t<C>, void> {
            using node_type = conditions::where_t<C>;
         
            template<class L>
            void operator()(const node_type &where, const L &l) const {
                iterate_ast(where.c, l);
            }
        };
        
        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, conditions::binary_condition>::value>::type> {
            using node_type = T;
            
            template<class L>
            void operator()(const node_type &binaryCondition, const L &l) const {
                iterate_ast(binaryCondition.l, l);
                iterate_ast(binaryCondition.r, l);
            }
        };
        
        template<class L, class R, class ...Ds>
        struct ast_iterator<binary_operator<L, R, Ds...>, void> {
            using node_type = binary_operator<L, R, Ds...>;
            
            template<class C>
            void operator()(const node_type &binaryOperator, const C &l) const {
                iterate_ast(binaryOperator.lhs, l);
                iterate_ast(binaryOperator.rhs, l);
            }
        };
        
        template<class ...Args>
        struct ast_iterator<columns_t<Args...>, void> {
            using node_type = columns_t<Args...>;
            
            template<class L>
            void operator()(const node_type &cols, const L &l) const {
                iterate_ast(cols.columns, l);
            }
        };
        
        template<class L, class A>
        struct ast_iterator<conditions::in_t<L, A>, void> {
            using node_type = conditions::in_t<L, A>;
            
            template<class C>
            void operator()(const node_type &in, const C &l) const {
                iterate_ast(in.l, l);
                iterate_ast(in.arg, l);
            }
        };
        
        template<class T>
        struct ast_iterator<std::vector<T>, void> {
            using node_type = std::vector<T>;
            
            template<class L>
            void operator()(const node_type &vec, const L &l) const {
                for(auto &i : vec) {
                    iterate_ast(i, l);
                }
            }
        };
        
        template<>
        struct ast_iterator<std::vector<char>, void> {
            using node_type = std::vector<char>;
            
            template<class L>
            void operator()(const node_type &vec, const L &l) const {
                l(vec);
            }
        };
        
        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, compound_operator>::value>::type> {
            using node_type = T;
            
            template<class L>
            void operator()(const node_type &c, const L &l) const {
                iterate_ast(c.left, l);
                iterate_ast(c.right, l);
            }
        };
        
        template<class T, class ...Args>
        struct ast_iterator<select_t<T, Args...>, void> {
            using node_type = select_t<T, Args...>;
            
            template<class L>
            void operator()(const node_type &sel, const L &l) const {
                iterate_ast(sel.col, l);
                iterate_ast(sel.conditions, l);
            }
        };
        
        template<class ...Args>
        struct ast_iterator<std::tuple<Args...>, void> {
            using node_type = std::tuple<Args...>;
            
            template<class L>
            void operator()(const node_type &tuple, const L &l) const {
                iterate_tuple(tuple, [&l](auto &v){
                    iterate_ast(v, l);
                });
            }
        };
        
        template<class T>
        struct ast_iterator<conditions::having_t<T>, void> {
            using node_type = conditions::having_t<T>;
            
            template<class L>
            void operator()(const node_type &hav, const L &l) const {
                iterate_ast(hav.t, l);
            }
        };
        
        template<class T, class E>
        struct ast_iterator<conditions::cast_t<T, E>, void> {
            using node_type = conditions::cast_t<T, E>;
            
            template<class L>
            void operator()(const node_type &c, const L &l) const {
                iterate_ast(c.expression, l);
            }
        };
        
        template<class T>
        struct ast_iterator<conditions::exists_t<T>, void> {
            using node_type = conditions::exists_t<T>;
            
            template<class L>
            void operator()(const node_type &e, const L &l) const {
                iterate_ast(e.t, l);
            }
        };
        
        template<class A, class T, class E>
        struct ast_iterator<conditions::like_t<A, T, E>, void> {
            using node_type = conditions::like_t<A, T, E>;
            
            template<class L>
            void operator()(const node_type &lk, const L &l) const {
                iterate_ast(lk.arg, l);
                iterate_ast(lk.pattern, l);
                lk.arg3.apply([&l](auto &value){
                    iterate_ast(value, l);
                });
            }
        };
        
        template<class A, class T>
        struct ast_iterator<conditions::between_t<A, T>, void> {
            using node_type = conditions::between_t<A, T>;
            
            template<class L>
            void operator()(const node_type &b, const L &l) const {
                iterate_ast(b.expr, l);
                iterate_ast(b.b1, l);
                iterate_ast(b.b2, l);
            }
        };
        
        template<class T>
        struct ast_iterator<conditions::named_collate<T>, void> {
            using node_type = conditions::named_collate<T>;
            
            template<class L>
            void operator()(const node_type &col, const L &l) const {
                iterate_ast(col.expr, l);
            }
        };
        
        template<class C>
        struct ast_iterator<conditions::negated_condition_t<C>, void> {
            using node_type = conditions::negated_condition_t<C>;
            
            template<class L>
            void operator()(const node_type &neg, const L &l) const {
                iterate_ast(neg.c, l);
            }
        };
        
        template<class R, class S, class ...Args>
        struct ast_iterator<core_functions::core_function_t<R, S, Args...>, void> {
            using node_type = core_functions::core_function_t<R, S, Args...>;
            
            template<class L>
            void operator()(const node_type &f, const L &l) const {
                iterate_ast(f.args, l);
            }
        };
        
        template<class T, class O>
        struct ast_iterator<conditions::left_join_t<T, O>, void> {
            using node_type = conditions::left_join_t<T, O>;
            
            template<class L>
            void operator()(const node_type &j, const L &l) const {
                iterate_ast(j.constraint, l);
            }
        };
        
        template<class T>
        struct ast_iterator<conditions::on_t<T>, void> {
            using node_type = conditions::on_t<T>;
            
            template<class L>
            void operator()(const node_type &o, const L &l) const {
                iterate_ast(o.arg, l);
            }
        };
        
        template<class T, class O>
        struct ast_iterator<conditions::join_t<T, O>, void> {
            using node_type = conditions::join_t<T, O>;
            
            template<class L>
            void operator()(const node_type &j, const L &l) const {
                iterate_ast(j.constraint, l);
            }
        };
        
        template<class T, class O>
        struct ast_iterator<conditions::left_outer_join_t<T, O>, void> {
            using node_type = conditions::left_outer_join_t<T, O>;
            
            template<class L>
            void operator()(const node_type &j, const L &l) const {
                iterate_ast(j.constraint, l);
            }
        };
        
        template<class T, class O>
        struct ast_iterator<conditions::inner_join_t<T, O>, void> {
            using node_type = conditions::inner_join_t<T, O>;
            
            template<class L>
            void operator()(const node_type &j, const L &l) const {
                iterate_ast(j.constraint, l);
            }
        };
        
        template<class R, class T, class E, class ...Args>
        struct ast_iterator<simple_case_t<R, T, E, Args...>, void> {
            using node_type = simple_case_t<R, T, E, Args...>;
            
            template<class L>
            void operator()(const node_type &c, const L &l) const {
                c.case_expression.apply([&l](auto &c){
                    iterate_ast(c, l);
                });
                iterate_tuple(c.args, [&l](auto &pair){
                    iterate_ast(pair.first, l);
                    iterate_ast(pair.second, l);
                });
                c.else_expression.apply([&l](auto &el){
                    iterate_ast(el, l);
                });
            }
        };
        
        template<class T, class E>
        struct ast_iterator<as_t<T, E>, void> {
            using node_type = as_t<T, E>;
            
            template<class L>
            void operator()(const node_type &a, const L &l) const {
                iterate_ast(a.expression, l);
            }
        };
    }
}
