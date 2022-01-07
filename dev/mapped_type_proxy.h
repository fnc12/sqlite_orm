#pragma once

#include "alias.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  If T is alias then mapped_type_proxy<T>::type is alias::type
         *  otherwise T is T.
         */
        template<class T, class SFINAE = void>
        struct mapped_type_proxy {
            using type = T;
        };

        template<class T>
        struct mapped_type_proxy<T, typename std::enable_if<std::is_base_of<alias_tag, T>::value>::type> {
            using type = typename T::type;
        };
    }
}
