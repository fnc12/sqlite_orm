#pragma once

#include "conditions.h"
#include "select_constraints.h"
#include "operators.h"

#include <iostream>

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
                std::cout << "t is " << typeid(t).name() << std::endl;
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
                l(binaryCondition.l);
                l(binaryCondition.r);
            }
        };
        
        template<class T>
        struct ast_iterator<T, typename std::enable_if<is_base_of_template<T, binary_operator>::value>::type> {
            using node_type = T;
            
            template<class L>
            void operator()(const node_type &binaryOperator, const L &l) const {
                l(binaryOperator.l);
                l(binaryOperator.r);
            }
        };
        
        template<class L, class R>
        struct ast_iterator<assign_t<L, R>, void> {
            using node_type = assign_t<L, R>;
            
            template<class C>
            void operator()(const node_type &assign, const C &l) const {
                l(assign.r);
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
        
        /*template<class L, class R>
        struct ast_iterator<conc_t<L, R>, void> {
            using node_type = conc_t<L, R>;
            
            template<class C>
            void operator()(const node_type &conc, const C &l) const {
                l(conc.l);
                l(conc.r);
            }
        };*/
    }
}
