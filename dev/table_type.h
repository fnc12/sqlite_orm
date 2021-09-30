#pragma once

#include <type_traits>  //  std::enable_if, std::is_member_pointer, std::is_member_function_pointer

#include "member_traits/getter_traits.h"
#include "member_traits/setter_traits.h"
#include "member_traits/is_getter.h"
#include "member_traits/is_setter.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class F>
        struct column_pointer;

        /**
         *  Trait class used to define table mapped type by setter/getter/member
         *  T - member pointer
         *  `type` is a type which is mapped.
         *  E.g.
         *  -   `table_type<decltype(&User::id)>::type` is `User`
         *  -   `table_type<decltype(&User::getName)>::type` is `User`
         *  -   `table_type<decltype(&User::setName)>::type` is `User`
         */
        template<class T, class SFINAE = void>
        struct table_type;

        template<class O, class F>
        struct table_type<F O::*,
                          typename std::enable_if<std::is_member_pointer<F O::*>::value &&
                                                  !std::is_member_function_pointer<F O::*>::value>::type> {
            using type = O;
        };

        template<class T>
        struct table_type<T, typename std::enable_if<is_getter<T>::value>::type> {
            using type = typename getter_traits<T>::object_type;
        };

        template<class T>
        struct table_type<T, typename std::enable_if<is_setter<T>::value>::type> {
            using type = typename setter_traits<T>::object_type;
        };

        template<class T, class F>
        struct table_type<column_pointer<T, F>, void> {
            using type = T;
        };
    }
}
