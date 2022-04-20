#pragma once

#include <type_traits>

#include "cxx_polyfill.h"
#include "alias.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  If T is alias than mapped_type_proxy<T>::type is alias::type
         *  otherwise T is T.
         */
        template<class T, class SFINAE = void>
        struct mapped_type_proxy {
            using type = std::remove_const_t<T>;
        };

        template<class T>
        struct mapped_type_proxy<T, std::enable_if_t<std::is_base_of<alias_tag, T>::value>> {
            using type = std::remove_const_t<typename T::type>;
        };
    }
}
