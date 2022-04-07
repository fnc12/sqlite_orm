#pragma once

#include "type_traits.h"
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
        struct mapped_type_proxy<T, match_if<std::is_base_of, alias_tag, T>> {
            using type = type_t<T>;
        };
    }
}
