#pragma once

#include <type_traits>  //  std::enable_if

#include "is_field_member_pointer.h"
#include "is_getter.h"
#include "field_member_traits.h"
#include "is_setter.h"
#include "getter_traits.h"
#include "setter_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct member_traits;

        template<class T>
        struct member_traits<T, typename std::enable_if<is_field_member_pointer<T>::value>::type> {
            using object_type = typename field_member_traits<T>::object_type;
            using field_type = typename field_member_traits<T>::field_type;
        };

        template<class T>
        struct member_traits<T, typename std::enable_if<is_getter<T>::value>::type> {
            using object_type = typename getter_traits<T>::object_type;
            using field_type = typename getter_traits<T>::field_type;
        };

        template<class T>
        struct member_traits<T, typename std::enable_if<is_setter<T>::value>::type> {
            using object_type = typename setter_traits<T>::object_type;
            using field_type = typename setter_traits<T>::field_type;
        };
    }
}
