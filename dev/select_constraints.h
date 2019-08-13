#pragma once

#include <string>   //  std::string
#include <utility>  //  std::declval
#include <tuple>    //  std::tuple, std::get, std::tuple_size

#include "is_base_of_template.h"
#include "tuple_helper.h"
#include "optional_container.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  DISCTINCT generic container.
         */
        template<class T>
        struct distinct_t {
            T t;
            
            operator std::string() const {
                return "DISTINCT";
            }
        };
        
        /**
         *  ALL generic container.
         */
        template<class T>
        struct all_t {
            T t;
            
            operator std::string() const {
                return "ALL";
            }
        };
        
        template<class ...Args>
        struct columns_t {
            using columns_type = std::tuple<Args...>;
            
            columns_type columns;
            bool distinct = false;
            
            static constexpr const int count = std::tuple_size<columns_type>::value;
        };
        
        template<class ...Args>
        struct set_t {
            
            operator std::string() const {
                return "SET";
            }
            
            template<class F>
            void for_each(F) {
                //..
            }
        };
        
        template<class L, class ...Args>
        struct set_t<L, Args...> : public set_t<Args...> {
            static_assert(is_assign_t<typename std::remove_reference<L>::type>::value, "set_t argument must be assign_t");
            
            L l;
            
            using super = set_t<Args...>;
            using self = set_t<L, Args...>;
            
            set_t(L l_, Args&& ...args) : super(std::forward<Args>(args)...), l(std::forward<L>(l_)) {}
            
            template<class F>
            void for_each(F f) {
                f(l);
                this->super::for_each(f);
            }
        };
        
        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer {
            using type = T;
            using field_type = F;
            
            field_type field;
        };
        
        /**
         *  Subselect object type.
         */
        template<class T, class ...Args>
        struct select_t {
            using return_type = T;
            using conditions_type = std::tuple<Args...>;
            
            return_type col;
            conditions_type conditions;
            bool highest_level = false;
        };
        
        /**
         *  Base for UNION, UNION ALL, EXCEPT and INTERSECT
         */
        template<class L, class R>
        struct compound_operator {
            using left_type = L;
            using right_type = R;
            
            left_type left;
            right_type right;
            
            compound_operator(left_type l, right_type r): left(std::move(l)), right(std::move(r)) {
                this->left.highest_level = true;
                this->right.highest_level = true;
            }
        };
        
        struct union_base {
            bool all = false;
            
            operator std::string() const {
                if(!this->all){
                    return "UNION";
                }else{
                    return "UNION ALL";
                }
            }
        };
        
        /**
         *  UNION object type.
         */
        template<class L, class R>
        struct union_t : public compound_operator<L, R>, union_base {
            using left_type = typename compound_operator<L, R>::left_type;
            using right_type = typename compound_operator<L, R>::right_type;
            
            union_t(left_type l, right_type r, decltype(all) all): compound_operator<L, R>(std::move(l), std::move(r)), union_base{all} {}
            
            union_t(left_type l, right_type r): union_t(std::move(l), std::move(r), false) {}
        };
        
        /**
         *  EXCEPT object type.
         */
        template<class L, class R>
        struct except_t : public compound_operator<L, R> {
            using super = compound_operator<L, R>;
            using left_type = typename super::left_type;
            using right_type = typename super::right_type;
            
            using super::super;
            
            operator std::string() const {
                return "EXCEPT";
            }
        };
        
        /**
         *  INTERSECT object type.
         */
        template<class L, class R>
        struct intersect_t : public compound_operator<L, R> {
            using super = compound_operator<L, R>;
            using left_type = typename super::left_type;
            using right_type = typename super::right_type;
            
            using super::super;
            
            operator std::string() const {
                return "INTERSECT";
            }
        };
        
        /**
         *  Generic way to get DISTINCT value from any type.
         */
        template<class T>
        bool get_distinct(const T &) {
            return false;
        }
        
        template<class ...Args>
        bool get_distinct(const columns_t<Args...> &cols) {
            return cols.distinct;
        }
        
        template<class T>
        struct asterisk_t {
            using type = T;
        };
        
        template<class T>
        struct then_t {
            using expression_type = T;
            
            expression_type expression;
        };
        
        template<class R, class T, class E, class ...Args>
        struct simple_case_t {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;
            
            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;
        };
        
        /**
         *  T is a case expression type
         *  E is else type (void is ELSE is omitted)
         *  Args... is a pack of WHEN expressions
         */
        template<class R, class T, class E, class ...Args>
        struct simple_case_builder {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;
            
            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;
            
            template<class W, class Th>
            simple_case_builder<R, T, E, Args..., std::pair<W, Th>> when(W w, then_t<Th> t) {
                using result_args_type = std::tuple<Args..., std::pair<W, Th>>;
                result_args_type result_args;
                move_tuple<std::tuple_size<args_type>::value>(result_args, this->args);
                std::pair<W, Th> newPair{std::move(w), std::move(t.expression)};
                std::get<std::tuple_size<result_args_type>::value - 1>(result_args) = std::move(newPair);
                return {std::move(this->case_expression), std::move(result_args), std::move(this->else_expression)};
            }
            
            simple_case_t<R, T, E, Args...> end() {
                return {std::move(this->case_expression), std::move(args), std::move(this->else_expression)};
            }
            
            template<class El>
            simple_case_builder<R, T, El, Args...> else_(El el) {
                return {{std::move(this->case_expression)}, std::move(args), {std::move(el)}};
            }
        };
    }
    
    template<class T>
    internal::then_t<T> then(T t) {
        return {std::move(t)};
    }
    
    template<class R, class T>
    internal::simple_case_builder<R, T, void> case_(T t) {
        return {{std::move(t)}};
    }
    
    template<class R>
    internal::simple_case_builder<R, void, void> case_() {
        return {};
    }
    
    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {std::move(t)};
    }
    
    template<class T>
    internal::all_t<T> all(T t) {
        return {std::move(t)};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> distinct(internal::columns_t<Args...> cols) {
        cols.distinct = true;
        return cols;
    }
    
    /**
     *  SET keyword used in UPDATE ... SET queries.
     *  Args must have `assign_t` type. E.g. set(assign(&User::id, 5)) or set(c(&User::id) = 5)
     */
    template<class ...Args>
    internal::set_t<Args...> set(Args&& ...args) {
        return {std::forward<Args>(args)...};
    }
    
    template<class ...Args>
    internal::columns_t<Args...> columns(Args&& ...args) {
        return {std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }
    
    /**
     *  Use it like this:
     *  struct MyType : BaseType { ... };
     *  storage.select(column<MyType>(&BaseType::id));
     */
    template<class T, class F>
    internal::column_pointer<T, F> column(F f) {
        return {f};
    }
    
    /**
     *  Public function for subselect query. Is useful in UNION queries.
     */
    template<class T, class ...Args>
    internal::select_t<T, Args...> select(T t, Args ...args) {
        return {std::move(t), std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }
    
    /**
     *  Public function for UNION operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class L, class R>
    internal::union_t<L, R> union_(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }
    
    /**
     *  Public function for EXCEPT operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/except.cpp
     */
    template<class L, class R>
    internal::except_t<L, R> except(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }
    
    template<class L, class R>
    internal::intersect_t<L, R> intersect(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs)};
    }
    
    /**
     *  Public function for UNION ALL operator.
     *  lhs and rhs are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class L, class R>
    internal::union_t<L, R> union_all(L lhs, R rhs) {
        return {std::move(lhs), std::move(rhs), true};
    }
    
    template<class T>
    internal::asterisk_t<T> asterisk() {
        return {};
    }
}
