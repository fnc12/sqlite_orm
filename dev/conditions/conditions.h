
#ifndef conditions_h
#define conditions_h

#include <string>
#include <tuple>
#include <memory>

#include "collate_argument.h"
#include "constraints.h"

namespace sqlite_orm {
    
    namespace conditions {
        
        struct limit_t {
            int lim;
            bool has_offset = false;
            bool offset_is_implicit = false;
            int off = 0;
            
            limit_t(decltype(lim) lim_):lim(lim_){}
            
            limit_t(decltype(lim) lim_,
                    decltype(has_offset) has_offset_,
                    decltype(offset_is_implicit) offset_is_implicit_,
                    decltype(off) off_):
            lim(lim_),
            has_offset(has_offset_),
            offset_is_implicit(offset_is_implicit_),
            off(off_){}
            
            operator std::string () const {
                return "LIMIT";
            }
        };
        
        struct offset_t {
            int off;
        };
        
        /**
         *  Inherit from this class if target class can be chained with other conditions with '&&' and '||' operators
         */
        struct condition_t {};
        
        template<class T>
        struct collate_t : public condition_t {
            T expr;
            internal::collate_argument argument;
            
            collate_t(T expr_, internal::collate_argument argument_):expr(expr_),argument(argument_){}
            
            operator std::string () const {
                return internal::constraints::collate_t{this->argument};
            }
        };
        
        template<class C>
        struct negated_condition_t : public condition_t {
            C c;
            
            negated_condition_t(){}
            
            negated_condition_t(C c_):c(c_){}
            
            operator std::string () const {
                return "NOT";
            }
        };
        
        template<class L, class R>
        struct and_condition_t : public condition_t {
            L l;
            R r;
            
            and_condition_t(){}
            
            and_condition_t(L l_, R r_):l(l_),r(r_){}
            
            operator std::string () const {
                return "AND";
            }
        };
        
        template<class L, class R>
        struct or_condition_t : public condition_t {
            L l;
            R r;
            
            or_condition_t(){}
            
            or_condition_t(L l_, R r_):l(l_),r(r_){}
            
            operator std::string () const {
                return "OR";
            }
        };
        
        template<class L, class R>
        struct binary_condition : public condition_t {
            L l;
            R r;
            
            binary_condition(){}
            
            binary_condition(L l_, R r_):l(l_),r(r_){}
        };
        
        /**
         *  = and == operators object.
         */
        template<class L, class R>
        struct is_equal_t : public binary_condition<L, R> {
            
            using self_type = is_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return "=";
            }
            
            negated_condition_t<is_equal_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
            
        };
        
        /**
         *  != operator object.
         */
        template<class L, class R>
        struct is_not_equal_t : public binary_condition<L, R> {
            
            using self_type = is_not_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return "!=";
            }
            
            negated_condition_t<is_not_equal_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        /**
         *  > operator object.
         */
        template<class L, class R>
        struct greater_than_t : public binary_condition<L, R> {
            
            using self_type = greater_than_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return ">";
            }
            
            negated_condition_t<greater_than_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        /**
         *  >= operator object.
         */
        template<class L, class R>
        struct greater_or_equal_t : public binary_condition<L, R> {
            
            using self_type = greater_or_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return ">=";
            }
            
            negated_condition_t<greater_or_equal_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        /**
         *  < operator object.
         */
        template<class L, class R>
        struct lesser_than_t : public binary_condition<L, R> {
            
            using self_type = lesser_than_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return "<";
            }
            
            negated_condition_t<lesser_than_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        /**
         *  <= operator object.
         */
        template<class L, class R>
        struct lesser_or_equal_t : public binary_condition<L, R> {
            
            using self_type = lesser_or_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            operator std::string () const {
                return "<=";
            }
            
            negated_condition_t<lesser_or_equal_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self_type> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self_type> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self_type> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        template<class L, class E>
        struct in_t : public condition_t {
            L l;    //  left expression..
            std::vector<E> values;       //  values..
            
            in_t(L l_, std::vector<E> values_):l(l_), values(values_){}
            
            negated_condition_t<in_t<L, E>> operator!() const {
                return {*this};
            }
            
            operator std::string () const {
                return "IN";
            }
        };
        
        template<class T>
        struct is_null_t {
            T t;
            
            negated_condition_t<is_null_t<T>> operator!() const {
                return {*this};
            }
            
            operator std::string () const {
                return "IS NULL";
            }
        };
        
        template<class T>
        struct is_not_null_t {
            T t;
            
            negated_condition_t<is_not_null_t<T>> operator!() const {
                return {*this};
            }
            
            operator std::string () const {
                return "IS NOT NULL";
            }
        };
        
        template<class C>
        struct where_t {
            C c;
            
            operator std::string() const {
                return "WHERE";
            }
        };
        
        template<class O>
        struct order_by_t {
            using self_type = order_by_t<O>;
            
            O o;
            int asc_desc = 0;   //  1: asc, -1: desc
            std::shared_ptr<internal::collate_argument> _collate_argument;
            
            order_by_t():o(nullptr){}
            
            order_by_t(O o_):o(o_){}
            
            operator std::string() const {
                return "ORDER BY";
            }
            
            order_by_t<O> asc() {
                auto res = *this;
                res.asc_desc = 1;
                return res;
            }
            
            order_by_t<O> desc() {
                auto res = *this;
                res.asc_desc = -1;
                return res;
            }
            
            self_type collate_binary() const {
                auto res = *this;
                res._collate_argument = std::make_unique<internal::collate_argument>(internal::collate_argument::binary);
                return res;
            }
            
            self_type collate_nocase() const {
                auto res = *this;
                res._collate_argument = std::make_unique<internal::collate_argument>(internal::collate_argument::nocase);
                return res;
            }
            
            self_type collate_rtrim() const {
                auto res = *this;
                res._collate_argument = std::make_unique<internal::collate_argument>(internal::collate_argument::rtrim);
                return res;
            }
        };
        
        template<class ...Args>
        struct group_by_t {
            std::tuple<Args...> args;
            
            operator std::string() const {
                return "GROUP BY";
            }
        };
        
        template<class A, class T>
        struct between_t : public condition_t {
            A expr;
            T b1;
            T b2;
            
            between_t(A expr_, T b1_, T b2_):expr(expr_), b1(b1_), b2(b2_){}
            
            operator std::string() const {
                return "BETWEEN";
            }
        };
        
        template<class A, class T>
        struct like_t : public condition_t {
            A a;
            T t;
            
            like_t(A a_, T t_):a(a_), t(t_){}
            
            operator std::string() const {
                return "LIKE";
            }
        };
        
        template<class T>
        struct cross_join_t {
            using type = T;
            
            operator std::string() const {
                return "CROSS JOIN";
            }
        };
        
        template<class T, class O>
        struct left_join_t {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            operator std::string() const {
                return "LEFT JOIN";
            }
        };
        
        /*template<class T>
         struct left_join_t<T, void> {
         typedef T type;
         
         operator std::string() const {
         return "LEFT JOIN";
         }
         };*/
        
        template<class T, class O>
        struct join_t {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            operator std::string() const {
                return "JOIN";
            }
        };
        
        /*template<class T>
         struct natural_join_t {
         typedef T type;
         
         operator std::string() const {
         return "NATURAL JOIN";
         }
         };*/
        
        template<class T, class O>
        struct left_outer_join_t {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            operator std::string() const {
                return "LEFT OUTER JOIN";
            }
        };
        
        template<class T>
        struct on_t {
            T t;
            
            operator std::string() const {
                return "ON";
            }
        };
        
        template<class F, class O>
        struct using_t {
            F O::*column;
            
            operator std::string() const {
                return "USING";
            }
        };
        
        template<class T, class O>
        struct inner_join_t {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            operator std::string() const {
                return "INNER JOIN";
            }
        };
        
    }
}

#endif /* conditions_h */
