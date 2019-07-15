#pragma once

#include <type_traits>  //  std::false_type, std::true_type

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  Inherit this class to support arithmetic types overloading
         */
        struct arithmetic_t {};
        
        template<class L, class R, class ...Ds>
        struct binary_operator : Ds... {
            using left_type = L;
            using right_type = R;
            
            left_type lhs;
            right_type rhs;
            
            binary_operator(left_type lhs_, right_type rhs_) : lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
        };
        
        struct conc_string {
            operator std::string () const {
                return "||";
            }
        };
        
        /**
         *  Result of concatenation || operator
         */
        template<class L, class R>
        using conc_t = binary_operator<L, R, conc_string>;
        
        struct add_string {
            operator std::string () const {
                return "+";
            }
        };
        
        /**
         *  Result of addition + operator
         */
        template<class L, class R>
        using add_t = binary_operator<L, R, add_string, arithmetic_t>;
        
        struct sub_string {
            operator std::string () const {
                return "-";
            }
        };
        
        /**
         *  Result of substitute - operator
         */
        template<class L, class R>
        using sub_t = binary_operator<L, R, sub_string, arithmetic_t>;
        
        struct mul_string {
            operator std::string () const {
                return "*";
            }
        };
        
        /**
         *  Result of multiply * operator
         */
        template<class L, class R>
        using mul_t = binary_operator<L, R, mul_string, arithmetic_t>;
        
        struct div_string {
            operator std::string () const {
                return "/";
            }
        };
        
        /**
         *  Result of divide / operator
         */
        template<class L, class R>
        using div_t = binary_operator<L, R, div_string, arithmetic_t>;
        
        struct mod_string {
            operator std::string () const {
                return "%";
            }
        };
        
        /**
         *  Result of mod % operator
         */
        template<class L, class R>
        using mod_t = binary_operator<L, R, mod_string, arithmetic_t>;
        
        struct assign_string {
            operator std::string () const {
                return "=";
            }
        };
        
        /**
         *  Result of assign = operator
         */
        template<class L, class R>
        using assign_t = binary_operator<L, R, assign_string>;
        
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
     *  Public interface for || concatenation operator. Example: `select(conc(&User::name, "@gmail.com"));` => SELECT name || '@gmail.com' FROM users
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
