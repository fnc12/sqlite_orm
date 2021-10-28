#pragma once

#include <type_traits>  //  std::false_type, std::true_type, std::is_member_pointer, std::is_member_function_pointer

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct is_field_member_pointer : std::false_type {};

        template<class T>
        struct is_field_member_pointer<T,
                                       typename std::enable_if<std::is_member_pointer<T>::value &&
                                                               !std::is_member_function_pointer<T>::value>::type>
            : std::true_type {};
    }
}
