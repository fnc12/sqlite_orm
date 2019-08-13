#pragma once

#include <string>   //  std::string
#include <type_traits>  //  std::enable_if, std::is_same
#include <vector>   //  std::vector

#include "collate_argument.h"
#include "constraints.h"
#include "optional_container.h"

namespace sqlite_orm {
    
    namespace conditions {
        
        /**
         *  Stores LIMIT/OFFSET info
         */
        struct limit_t {
            int lim = 0;
            bool has_offset = false;
            bool offset_is_implicit = false;
            int off = 0;
            
            limit_t() = default;
            
            limit_t(decltype(lim) lim_): lim(lim_) {}
            
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
        
        /**
         *  Stores OFFSET only info
         */
        struct offset_t {
            int off;
        };
        
        /**
         *  Inherit from this class if target class can be chained with other conditions with '&&' and '||' operators
         */
        struct condition_t {};
        
        /**
         *  Collated something
         */
        template<class T>
        struct collate_t : public condition_t {
            T expr;
            internal::collate_argument argument;
            
            collate_t(T expr_, internal::collate_argument argument_): expr(expr_), argument(argument_) {}
            
            operator std::string () const {
                return constraints::collate_t{this->argument};
            }
        };
        
        struct named_collate_base {
            std::string name;
            
            operator std::string () const {
                return "COLLATE " + this->name;
            }
        };
        
        /**
         *  Collated something with custom collate function
         */
        template<class T>
        struct named_collate : named_collate_base {
            T expr;
            
            named_collate(T expr_, std::string name_): named_collate_base{std::move(name_)}, expr(std::move(expr_)) {}
        };
        
        struct negated_condition_string {
            operator std::string () const {
                return "NOT";
            }
        };
        
        /**
         *  Result of not operator
         */
        template<class C>
        struct negated_condition_t : condition_t, negated_condition_string {
            C c;
            
            negated_condition_t(C c_): c(std::move(c_)) {}
        };
        
        /**
         *  Base class for binary conditions
         */
        template<class L, class R>
        struct binary_condition : public condition_t {
            L l;
            R r;
            
            binary_condition() = default;
            
            binary_condition(L l_, R r_): l(std::move(l_)), r(std::move(r_)) {}
        };
        
        struct and_condition_string {
            operator std::string () const {
                return "AND";
            }
        };
        
        /**
         *  Result of and operator
         */
        template<class L, class R>
        struct and_condition_t : binary_condition<L, R>, and_condition_string {
            using super = binary_condition<L, R>;
            
            using super::super;
        };
        
        struct or_condition_string {
            operator std::string () const {
                return "OR";
            }
        };
        
        /**
         *  Result of or operator
         */
        template<class L, class R>
        struct or_condition_t : binary_condition<L, R>, or_condition_string {
            using super = binary_condition<L, R>;
            
            using super::super;
        };
        
        struct is_equal_string {
            operator std::string () const {
                return "=";
            }
        };
        
        /**
         *  = and == operators object
         */
        template<class L, class R>
        struct is_equal_t : binary_condition<L, R>, is_equal_string {
            using self = is_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
            
            named_collate<self> collate(std::string name) const {
                return {*this, std::move(name)};
            }
            
        };
        
        struct is_not_equal_string {
            operator std::string () const {
                return "!=";
            }
        };
        
        /**
         *  != operator object
         */
        template<class L, class R>
        struct is_not_equal_t : binary_condition<L, R>, is_not_equal_string {
            using self = is_not_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        struct greater_than_string {
            operator std::string () const {
                return ">";
            }
        };
        
        /**
         *  > operator object.
         */
        template<class L, class R>
        struct greater_than_t : binary_condition<L, R>, greater_than_string {
            using self = greater_than_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        struct greater_or_equal_string {
            operator std::string () const {
                return ">=";
            }
        };
        
        /**
         *  >= operator object.
         */
        template<class L, class R>
        struct greater_or_equal_t : binary_condition<L, R>, greater_or_equal_string {
            using self = greater_or_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        struct lesser_than_string {
            operator std::string () const {
                return "<";
            }
        };
        
        /**
         *  < operator object.
         */
        template<class L, class R>
        struct lesser_than_t : binary_condition<L, R>, lesser_than_string {
            using self = lesser_than_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        struct lesser_or_equal_string {
            operator std::string () const {
                return "<=";
            }
        };
        
        /**
         *  <= operator object.
         */
        template<class L, class R>
        struct lesser_or_equal_t : binary_condition<L, R>, lesser_or_equal_string {
            using self = lesser_or_equal_t<L, R>;
            
            using binary_condition<L, R>::binary_condition;
            
            negated_condition_t<lesser_or_equal_t<L, R>> operator!() const {
                return {*this};
            }
            
            collate_t<self> collate_binary() const {
                return {*this, internal::collate_argument::binary};
            }
            
            collate_t<self> collate_nocase() const {
                return {*this, internal::collate_argument::nocase};
            }
            
            collate_t<self> collate_rtrim() const {
                return {*this, internal::collate_argument::rtrim};
            }
        };
        
        struct in_base {
            bool negative = false;  //  used in not_in
            
            operator std::string () const {
                if(!this->negative){
                    return "IN";
                }else{
                    return "NOT IN";
                }
            }
        };
        
        /**
         *  IN operator object.
         */
        template<class L, class A>
        struct in_t : condition_t, in_base {
            using self = in_t<L, A>;
            
            L l;    //  left expression
            A arg;       //  in arg
            
            in_t(L l_, A arg_, bool negative): in_base{negative}, l(l_), arg(std::move(arg_)) {}
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
        };
        
        struct is_null_string {
            operator std::string () const {
                return "IS NULL";
            }
        };
        
        /**
         *  IS NULL operator object.
         */
        template<class T>
        struct is_null_t : is_null_string {
            using self = is_null_t<T>;
            
            T t;
            
            is_null_t(T t_) : t(std::move(t_)) {}
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
        };
        
        struct is_not_null_string {
            operator std::string () const {
                return "IS NOT NULL";
            }
        };
        
        /**
         *  IS NOT NULL operator object.
         */
        template<class T>
        struct is_not_null_t : is_not_null_string {
            using self = is_not_null_t<T>;
            
            T t;
            
            is_not_null_t(T t_) : t(std::move(t_)) {}
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
        };
        
        struct where_string {
            operator std::string () const {
                return "WHERE";
            }
        };
        
        /**
         *  WHERE argument holder.
         *  C is conditions type. Can be any condition like: is_equal_t, is_null_t, exists_t etc
         */
        template<class C>
        struct where_t : where_string {
            C c;
            
            where_t(C c_) : c(std::move(c_)) {}
        };
        
        struct order_by_base {
            int asc_desc = 0;   //  1: asc, -1: desc
            std::string _collate_argument;
        };
        
        struct order_by_string {
            operator std::string() const {
                return "ORDER BY";
            }
        };
        
        /**
         *  ORDER BY argument holder.
         */
        template<class O>
        struct order_by_t : order_by_base, order_by_string {
            using self = order_by_t<O>;
            
            O o;
            
            order_by_t(O o_): o(std::move(o_)) {}
            
            self asc() {
                auto res = *this;
                res.asc_desc = 1;
                return res;
            }
            
            self desc() {
                auto res = *this;
                res.asc_desc = -1;
                return res;
            }
            
            self collate_binary() const {
                auto res = *this;
                res._collate_argument = constraints::collate_t::string_from_collate_argument(internal::collate_argument::binary);
                return res;
            }
            
            self collate_nocase() const {
                auto res = *this;
                res._collate_argument = constraints::collate_t::string_from_collate_argument(internal::collate_argument::nocase);
                return res;
            }
            
            self collate_rtrim() const {
                auto res = *this;
                res._collate_argument = constraints::collate_t::string_from_collate_argument(internal::collate_argument::rtrim);
                return res;
            }
            
            self collate(std::string name) const {
                auto res = *this;
                res._collate_argument = std::move(name);
                return res;
            }
        };
        
        /**
         *  ORDER BY pack holder.
         */
        template<class ...Args>
        struct multi_order_by_t : order_by_string {
            using args_type = std::tuple<Args...>;
            
            args_type args;
            
            multi_order_by_t(args_type &&args_) : args(std::move(args_)) {}
        };
        
        /**
         *  S - storage class
         */
        template<class S>
        struct dynamic_order_by_t : order_by_string {
            using storage_type = S;
            
            struct entry_t : order_by_base {
                std::string name;
                
                entry_t(decltype(name) name_, int asc_desc, std::string collate_argument) :
                order_by_base{asc_desc, move(collate_argument)},
                name(move(name_))
                {}
            };
            
            using const_iterator = typename std::vector<entry_t>::const_iterator;
            
            dynamic_order_by_t(const storage_type &storage_): storage(storage_) {}
            
            template<class O>
            void push_back(order_by_t<O> order_by) {
                auto columnName = this->storage.string_from_expression(order_by.o, true);
                entries.emplace_back(move(columnName), order_by.asc_desc, move(order_by._collate_argument));
            }
            
            const_iterator begin() const {
                return this->entries.begin();
            }
            
            const_iterator end() const {
                return this->entries.end();
            }
            
            void clear() {
                this->entries.clear();
            }
            
        protected:
            std::vector<entry_t> entries;
            const storage_type &storage;
        };
        
        struct group_by_string {
            operator std::string() const {
                return "GROUP BY";
            }
        };
        
        /**
         *  GROUP BY pack holder.
         */
        template<class ...Args>
        struct group_by_t : group_by_string {
            using args_type = std::tuple<Args...>;
            args_type args;
            
            group_by_t(args_type &&args_): args(std::move(args_)) {}
        };
        
        struct between_string {
            operator std::string() const {
                return "BETWEEN";
            }
        };
        
        /**
         *  BETWEEN operator object.
         */
        template<class A, class T>
        struct between_t : condition_t, between_string {
            A expr;
            T b1;
            T b2;
            
            between_t(A expr_, T b1_, T b2_): expr(std::move(expr_)), b1(std::move(b1_)), b2(std::move(b2_)) {}
        };
        
        struct like_string {
            operator std::string() const {
                return "LIKE";
            }
        };
        
        /**
         *  LIKE operator object.
         */
        template<class A, class T, class E>
        struct like_t : condition_t, like_string {
            using arg_t = A;
            using pattern_t = T;
            using escape_t = E;
            
            arg_t arg;
            pattern_t pattern;
            internal::optional_container<escape_t> arg3;  //  not escape cause escape exists as a function here
            
            like_t(arg_t arg_, pattern_t pattern_, internal::optional_container<escape_t> escape):
            arg(std::move(arg_)), pattern(std::move(pattern_)), arg3(std::move(escape)) {}
            
            template<class C>
            like_t<A, T, C> escape(C c) const {
                internal::optional_container<C> arg3{std::move(c)};
                return {std::move(this->arg), std::move(this->pattern), std::move(arg3)};
            }
        };
        
        struct cross_join_string {
            operator std::string() const {
                return "CROSS JOIN";
            }
        };
        
        /**
         *  CROSS JOIN holder.
         *  T is joined type which represents any mapped table.
         */
        template<class T>
        struct cross_join_t : cross_join_string {
            using type = T;
        };
        
        struct natural_join_string {
            operator std::string() const {
                return "NATURAL JOIN";
            }
        };
        
        /**
         *  NATURAL JOIN holder.
         *  T is joined type which represents any mapped table.
         */
        template<class T>
        struct natural_join_t : natural_join_string {
            using type = T;
        };
        
        struct left_join_string {
            operator std::string() const {
                return "LEFT JOIN";
            }
        };
        
        /**
         *  LEFT JOIN holder.
         *  T is joined type which represents any mapped table.
         *  O is on(...) argument type.
         */
        template<class T, class O>
        struct left_join_t : left_join_string {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            left_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
        };
        
        struct join_string {
            operator std::string() const {
                return "JOIN";
            }
        };
        
        /**
         *  Simple JOIN holder.
         *  T is joined type which represents any mapped table.
         *  O is on(...) argument type.
         */
        template<class T, class O>
        struct join_t : join_string {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
        };
        
        struct left_outer_join_string {
            operator std::string() const {
                return "LEFT OUTER JOIN";
            }
        };
        
        /**
         *  LEFT OUTER JOIN holder.
         *  T is joined type which represents any mapped table.
         *  O is on(...) argument type.
         */
        template<class T, class O>
        struct left_outer_join_t : left_outer_join_string {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            left_outer_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
        };
        
        struct on_string {
            operator std::string() const {
                return "ON";
            }
        };
        
        /**
         *  on(...) argument holder used for JOIN, LEFT JOIN, LEFT OUTER JOIN and INNER JOIN
         *  T is on type argument.
         */
        template<class T>
        struct on_t : on_string {
            using arg_type = T;
            
            arg_type arg;
            
            on_t(arg_type arg_) : arg(std::move(arg_)) {}
        };
        
        /**
         *  USING argument holder.
         */
        template<class F, class O>
        struct using_t {
            F O::*column = nullptr;
            
            operator std::string() const {
                return "USING";
            }
        };
        
        struct inner_join_string {
            operator std::string() const {
                return "INNER JOIN";
            }
        };
        
        /**
         *  INNER JOIN holder.
         *  T is joined type which represents any mapped table.
         *  O is on(...) argument type.
         */
        template<class T, class O>
        struct inner_join_t : inner_join_string {
            using type = T;
            using on_type = O;
            
            on_type constraint;
            
            inner_join_t(on_type constraint_) : constraint(std::move(constraint_)) {}
        };
        
        struct exists_string {
            operator std::string() const {
                return "EXISTS";
            }
        };
        
        template<class T>
        struct exists_t : condition_t, exists_string {
            using type = T;
            using self = exists_t<type>;
            
            type t;
            
            exists_t(T t_) : t(std::move(t_)) {}
            
            negated_condition_t<self> operator!() const {
                return {*this};
            }
        };
        
        struct having_string {
            operator std::string() const {
                return "HAVING";
            }
        };
        
        /**
         *  HAVING holder.
         *  T is having argument type.
         */
        template<class T>
        struct having_t : having_string {
            using type = T;
            
            type t;
            
            having_t(type t_) : t(std::move(t_)) {}
        };
    
        struct cast_string {
            operator std::string() const {
                return "CAST";
            }
        };
        
        template<class T, class E>
        struct cast_t : cast_string {
            using to_type = T;
            using expression_type = E;
            
            expression_type expression;
            
            cast_t(expression_type expression_) : expression(std::move(expression_)) {}
        };
        
    }
    
    /**
     *  Cute operators for columns
     */
    template<class T, class R>
    conditions::lesser_than_t<T, R> operator<(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::lesser_than_t<L, T> operator<(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::lesser_or_equal_t<T, R> operator<=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::lesser_or_equal_t<L, T> operator<=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::greater_than_t<T, R> operator>(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::greater_than_t<L, T> operator>(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::greater_or_equal_t<T, R> operator>=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::greater_or_equal_t<L, T> operator>=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::is_equal_t<T, R> operator==(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::is_equal_t<L, T> operator==(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    conditions::is_not_equal_t<T, R> operator!=(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    conditions::is_not_equal_t<L, T> operator!=(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class T, class R>
    internal::conc_t<T, R> operator||(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::conc_t<L, T> operator||(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::conc_t<L, R> operator||(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class T, class R>
    internal::add_t<T, R> operator+(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::add_t<L, T> operator+(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::add_t<L, R> operator+(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class T, class R>
    internal::sub_t<T, R> operator-(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::sub_t<L, T> operator-(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::sub_t<L, R> operator-(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class T, class R>
    internal::mul_t<T, R> operator*(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::mul_t<L, T> operator*(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::mul_t<L, R> operator*(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class T, class R>
    internal::div_t<T, R> operator/(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::div_t<L, T> operator/(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::div_t<L, R> operator/(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class T, class R>
    internal::mod_t<T, R> operator%(internal::expression_t<T> expr, R r) {
        return {expr.t, r};
    }
    
    template<class L, class T>
    internal::mod_t<L, T> operator%(L l, internal::expression_t<T> expr) {
        return {l, expr.t};
    }
    
    template<class L, class R>
    internal::mod_t<L, R> operator%(internal::expression_t<L> l, internal::expression_t<R> r) {
        return {l.t, r.t};
    }
    
    template<class F, class O>
    conditions::using_t<F, O> using_(F O::*p) {
        return {p};
    }
    
    template<class T>
    conditions::on_t<T> on(T t) {
        return {t};
    }
    
    template<class T>
    conditions::cross_join_t<T> cross_join() {
        return {};
    }
    
    template<class T>
    conditions::natural_join_t<T> natural_join() {
        return {};
    }
    
    template<class T, class O>
    conditions::left_join_t<T, O> left_join(O o) {
        return {o};
    }
    
    template<class T, class O>
    conditions::join_t<T, O> join(O o) {
        return {o};
    }
    
    template<class T, class O>
    conditions::left_outer_join_t<T, O> left_outer_join(O o) {
        return {o};
    }
    
    template<class T, class O>
    conditions::inner_join_t<T, O> inner_join(O o) {
        return {o};
    }
    
    inline conditions::offset_t offset(int off) {
        return {off};
    }
    
    inline conditions::limit_t limit(int lim) {
        return {lim};
    }
    
    inline conditions::limit_t limit(int off, int lim) {
        return {lim, true, true, off};
    }
    
    inline conditions::limit_t limit(int lim, conditions::offset_t offt) {
        return {lim, true, false, offt.off };
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<std::is_base_of<conditions::condition_t, L>::value || std::is_base_of<conditions::condition_t, R>::value>::type
    >
    conditions::and_condition_t<L, R> operator &&(const L &l, const R &r) {
        return {l, r};
    }
    
    template<
    class L,
    class R,
    typename = typename std::enable_if<std::is_base_of<conditions::condition_t, L>::value || std::is_base_of<conditions::condition_t, R>::value>::type
    >
    conditions::or_condition_t<L, R> operator ||(const L &l, const R &r) {
        return {l, r};
    }
    
    template<class T>
    conditions::is_not_null_t<T> is_not_null(T t) {
        return {t};
    }
    
    template<class T>
    conditions::is_null_t<T> is_null(T t) {
        return {t};
    }
    
    template<class L, class E>
    conditions::in_t<L, std::vector<E>> in(L l, std::vector<E> values) {
        return {std::move(l), std::move(values), false};
    }
    
    template<class L, class E>
    conditions::in_t<L, std::vector<E>> in(L l, std::initializer_list<E> values) {
        return {std::move(l), std::move(values), false};
    }
    
    template<class L, class A>
    conditions::in_t<L, A> in(L l, A arg) {
        return {std::move(l), std::move(arg), false};
    }
    
    template<class L, class E>
    conditions::in_t<L, std::vector<E>> not_in(L l, std::vector<E> values) {
        return {std::move(l), std::move(values), true};
    }
    
    template<class L, class E>
    conditions::in_t<L, std::vector<E>> not_in(L l, std::initializer_list<E> values) {
        return {std::move(l), std::move(values), true};
    }
    
    template<class L, class A>
    conditions::in_t<L, A> not_in(L l, A arg) {
        return {std::move(l), std::move(arg), true};
    }
    
    template<class L, class R>
    conditions::is_equal_t<L, R> is_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_equal_t<L, R> eq(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_not_equal_t<L, R> is_not_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::is_not_equal_t<L, R> ne(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_than_t<L, R> greater_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_than_t<L, R> gt(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_or_equal_t<L, R> greater_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::greater_or_equal_t<L, R> ge(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_than_t<L, R> lesser_than(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_than_t<L, R> lt(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_or_equal_t<L, R> lesser_or_equal(L l, R r) {
        return {l, r};
    }
    
    template<class L, class R>
    conditions::lesser_or_equal_t<L, R> le(L l, R r) {
        return {l, r};
    }
    
    template<class C>
    conditions::where_t<C> where(C c) {
        return {c};
    }
    
    template<class O>
    conditions::order_by_t<O> order_by(O o) {
        return {o};
    }
    
    template<class ...Args>
    conditions::multi_order_by_t<Args...> multi_order_by(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    template<class S>
    conditions::dynamic_order_by_t<S> dynamic_order_by(const S &storage) {
        return {storage};
    }
    
    template<class ...Args>
    conditions::group_by_t<Args...> group_by(Args&& ...args) {
        return {std::make_tuple(std::forward<Args>(args)...)};
    }
    
    template<class A, class T>
    conditions::between_t<A, T> between(A expr, T b1, T b2) {
        return {expr, b1, b2};
    }
    
    template<class A, class T>
    conditions::like_t<A, T, void> like(A a, T t) {
        return {std::move(a), std::move(t), {}};
    }
    
    template<class A, class T, class E>
    conditions::like_t<A, T, E> like(A a, T t, E e) {
        return {std::move(a), std::move(t), {std::move(e)}};
    }
    
    template<class T>
    conditions::exists_t<T> exists(T t) {
        return {std::move(t)};
    }
    
    template<class T>
    conditions::having_t<T> having(T t) {
        return {t};
    }
    
    template<class T, class E>
    conditions::cast_t<T, E> cast(E e) {
        return {e};
    }
}
