#pragma once

#include <type_traits>  //  std::enable_if, std::is_base_of, std::remove_const

#include "alias.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  If T is alias than mapped_type_proxy<T>::type is alias::type
         *  otherwise T is unqualified T.
         */
        template<class T, class SFINAE = void>
        struct mapped_type_proxy : std::remove_const<T> {};

        template<class T>
        struct mapped_type_proxy<T, std::enable_if_t<std::is_base_of<alias_tag, T>::value>> {
            using type = typename T::type;
        };

        template<class T>
        using mapped_type_proxy_t = typename mapped_type_proxy<T>::type;
    }
}
