#pragma once

#include <type_traits>  //  std::remove_const

#include "type_traits.h"
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
        struct mapped_type_proxy<T, match_if<is_table_alias, T>> {
            using type = type_t<T>;
        };

        template<class T>
        using mapped_type_proxy_t = typename mapped_type_proxy<T>::type;
    }
}
