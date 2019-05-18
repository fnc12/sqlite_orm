#include <type_traits>  //  std::true_type, std::false_type, std::is_same, std::enable_if

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

        // from https://stackoverflow.com/questions/22213523/c11-14-how-to-remove-a-pointer-to-member-from-a-type
        template<class T> struct remove_member_pointer {
            typedef T type;
        };

        template<class C, class T> struct remove_member_pointer<T C::*> {
            typedef T type;
        };

        template<class C, bool IsMemberPointer>
        struct column_value_type;

        template<class C>
        struct column_value_type<C, true> {
            typedef typename remove_member_pointer<C>::type type;
        };

        template<class C>
        struct column_value_type<C, false> {
            typedef typename getter_traits<C>::field_type type;
        };
    }
}