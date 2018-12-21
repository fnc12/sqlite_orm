#include <type_traits>  //  std::is_same, std::enable_if, std::true_type, std::false_type, std::integral_constant

namespace sqlite_orm {
    
    namespace internal {
        
        namespace storage_traits {
            
            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S, class T, class SFINAE = void>
            struct type_is_mapped_impl;
            
            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class T>
            struct type_is_mapped : type_is_mapped_impl<typename S::impl_type, T> {};
            
            template<class T>
            struct type_is_mapped_impl<storage_impl<>, T, void> : std::false_type {};
            
            template<class S, class T>
            struct type_is_mapped_impl<S, T, typename std::enable_if<std::is_same<T, typename S::table_type::object_type>::value>::type> : std::true_type {};
            
            template<class S, class T>
            struct type_is_mapped_impl<S, T, typename std::enable_if<!std::is_same<T, typename S::table_type::object_type>::value>::type>
            : type_is_mapped_impl<typename S::super, T> {};
        }
    }
}
