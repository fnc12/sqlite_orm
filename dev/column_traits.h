#pragma once

#include <type_traits>  //  std::true_type, std::false_type, std::is_same, std::enable_if

#include "select_constraints.h"

namespace sqlite_orm {
    
    namespace internal {
        /**
         *  Getters aliases
         */
        template<class O, class T>
        using getter_by_value_const = T (O::*)() const;
        
        template<class O, class T>
        using getter_by_value = T (O::*)();
        
        template<class O, class T>
        using getter_by_ref_const = T& (O::*)() const;
        
        template<class O, class T>
        using getter_by_ref = T& (O::*)();
        
        template<class O, class T>
        using getter_by_const_ref_const = const T& (O::*)() const;
        
        template<class O, class T>
        using getter_by_const_ref = const T& (O::*)();
        
        /**
         *  Setters aliases
         */
        template<class O, class T>
        using setter_by_value = void (O::*)(T);
        
        template<class O, class T>
        using setter_by_ref = void (O::*)(T&);
        
        template<class O, class T>
        using setter_by_const_ref = void (O::*)(const T&);
        
        template<class T>
        struct is_getter : std::false_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_value_const<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_value<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_ref_const<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_ref<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_const_ref_const<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_getter<getter_by_const_ref<O, T>> : std::true_type {};
        
        template<class T>
        struct is_setter : std::false_type {};
        
        template<class O, class T>
        struct is_setter<setter_by_value<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_setter<setter_by_ref<O, T>> : std::true_type {};
        
        template<class O, class T>
        struct is_setter<setter_by_const_ref<O, T>> : std::true_type {};
        
        template<class T>
        struct getter_traits;
        
        template<class O, class T>
        struct getter_traits<getter_by_value_const<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = false;
        };
        
        template<class O, class T>
        struct getter_traits<getter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = false;
        };
        
        template<class O, class T>
        struct getter_traits<getter_by_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = true;
        };
        
        template<class O, class T>
        struct getter_traits<getter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = true;
        };
        
        template<class O, class T>
        struct getter_traits<getter_by_const_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = true;
        };
        
        template<class O, class T>
        struct getter_traits<getter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;
            
            static constexpr const bool returns_lvalue = true;
        };
        
        template<class T>
        struct setter_traits;
        
        template<class O, class T>
        struct setter_traits<setter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;
        };
        
        template<class O, class T>
        struct setter_traits<setter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };
        
        template<class O, class T>
        struct setter_traits<setter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        /**
         *  Trait class used to define table mapped type by setter/getter/member
         *  T - member pointer
         */
        template<class T, class SFINAE = void>
        struct column_traits;
        
        template<class O, class F>
        struct column_traits<F O::*, typename std::enable_if<std::is_member_pointer<F O::*>::value && !std::is_member_function_pointer<F O::*>::value>::type> {
            using type = O;
            using field_type = F;
        };
        
        template<class T>
        struct column_traits<T, typename std::enable_if<is_getter<T>::value>::type> {
            using type = typename getter_traits<T>::object_type;
            using field_type = typename getter_traits<T>::field_type;
        };
        
        template<class T>
        struct column_traits<T, typename std::enable_if<is_setter<T>::value>::type> {
            using type = typename setter_traits<T>::object_type;
            using field_type = typename setter_traits<T>::field_type;
        };
        
        template<class T, class F>
        struct column_traits<column_pointer<T, F>, void> {
            using type = T;
            using field_type = F;
        };
    }
}
