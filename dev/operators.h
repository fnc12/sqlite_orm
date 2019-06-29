#pragma once

#include <type_traits>  //  std::false_type, std::true_type

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  Inherit this class to support arithmetic types overloading
         */
        struct arithmetic_t {};
        
        template<class L, class R>
        struct binary_operator {
            using left_type = L;
            using right_type = R;
            
            left_type lhs;
            right_type rhs;
            
            binary_operator(left_type lhs_, right_type rhs_) : lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
        };
        
        /**
         *  Result of concatenation || operator
         */
        template<class L, class R>
        struct conc_t : binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of addition + operator
         */
        template<class L, class R>
        struct add_t : arithmetic_t, binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of substitute - operator
         */
        template<class L, class R>
        struct sub_t : arithmetic_t, binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of multiply * operator
         */
        template<class L, class R>
        struct mul_t : arithmetic_t, binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of divide / operator
         */
        template<class L, class R>
        struct div_t : arithmetic_t, binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of mod % operator
         */
        template<class L, class R>
        struct mod_t : arithmetic_t, binary_operator<L, R> {
            using super = binary_operator<L, R>;
            
            using super::super;
        };
        
        /**
         *  Result of assign = operator
         */
        template<class L, class R>
        struct assign_t {
            L l;
            R r;
            
            assign_t(L l_, R r_): l(std::move(l_)), r(std::move(r_)) {}
        };
        
        /**
         *  Assign operator traits. Common case
         */
        template<class T>
        struct is_assign_t : public std::false_type {};
        
        /**
         *  Assign operator traits. Specialized case
         */
        template<class L, class R>
        struct is_assign_t<assign_t<L, R>> : public std::true_type {};
        
        /**
         *  Is not an operator but a result of c(...) function. Has operator= overloaded which returns assign_t
         */
        template<class T>
        struct expression_t {
            T t;
            
            expression_t(T t_): t(std::move(t_)) {}
            
            template<class R>
            assign_t<T, R> operator=(R r) const {
                return {this->t, std::move(r)};
            }
            
            assign_t<T, std::nullptr_t> operator=(std::nullptr_t) const {
                return {this->t, nullptr};
            }
        };
        
    }
    
    /**
     *  Public interface for syntax sugar for columns. Example: `where(c(&User::id) == 5)` or `storage.update(set(c(&User::name) = "Dua Lipa"));
     */
    template<class T>
    internal::expression_t<T> c(T t) {
        return {std::move(t)};
    }
    
    /**
     *  Public interface for || concatenation operator. Example: `select(conc(&User::name, "@gmail.com"));` => SELECT name + '@gmail.com' FROM users
     */
    template<class L, class R>
    internal::conc_t<L, R> conc(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    /**
     *  Public interface for + operator. Example: `select(add(&User::age, 100));` => SELECT age + 100 FROM users
     */
    template<class L, class R>
    internal::add_t<L, R> add(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    /**
     *  Public interface for - operator. Example: `select(add(&User::age, 1));` => SELECT age - 1 FROM users
     */
    template<class L, class R>
    internal::sub_t<L, R> sub(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<class L, class R>
    internal::mul_t<L, R> mul(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<class L, class R>
    internal::div_t<L, R> div(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<class L, class R>
    internal::mod_t<L, R> mod(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
    template<class L, class R>
    internal::assign_t<L, R> assign(L l, R r) {
        return {std::move(l), std::move(r)};
    }
    
}
