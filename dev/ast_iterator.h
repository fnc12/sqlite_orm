#pragma once

#include <vector>   //  std::vector

#include "conditions.h"
#include "select_constraints.h"
#include "operators.h"
#include "tuple_helper.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
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
        
        template<class T, class L>
        void iterate_ast(const T &t, const L &l) {
            ast_iterator<T> iterator;
            iterator(t, l);
        }
        
        template<class C>
        struct ast_iterator<conditions::where_t<C>, void> {
            using node_type = conditions::where_t<C>;
         
             //  L is a callable type. Mostly is templated lambda
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
        
        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, binary_operator>::value>::type> {
            using node_type = T;
            
            template<class L>
            void operator()(const node_type &binaryOperator, const L &l) const {
                iterate_ast(binaryOperator.l, l);
                iterate_ast(binaryOperator.r, l);
            }
        };
        
        template<class L, class R>
        struct ast_iterator<assign_t<L, R>, void> {
            using node_type = assign_t<L, R>;
            
            template<class C>
            void operator()(const node_type &assign, const C &l) const {
                iterate_ast(assign.r, l);
            }
        };
        
        template<class ...Args>
        struct ast_iterator<columns_t<Args...>, void> {
            using node_type = columns_t<Args...>;
            
            template<class L>
            void operator()(const node_type &cols, const L &l) const {
                cols.for_each([&l](auto &col){
                    iterate_ast(col, l);
                });
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
                tuple_helper::iterator<std::tuple_size<node_type>::value - 1, Args...>()(tuple, [&l](auto &v){
                    iterate_ast(v, l);
                }, false);
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
        
        /*template<class A, class T>
        struct ast_iterator<conditions::like_t<A, T>, void> {
            using node_type = conditions::like_t<A, T>;
            
            template<class L>
            void operator()(const node_type &lk, const L &l) const {
                iterate_ast(lk.a, l);
                iterate_ast(lk.t, l);
            }
        };*/
        
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
    }
}
