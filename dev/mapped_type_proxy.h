#pragma once

#include <type_traits>  //  std::remove_const

#include "type_traits.h"
#include "alias_traits.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  If T is a recordset alias then the typename mapped_type_proxy<T>::type is the unqualified aliased type,
         *  otherwise unqualified T.
         */
        template<class T, class SFINAE = void>
        struct mapped_type_proxy : std::remove_const<T> {};

        template<class T>
        struct mapped_type_proxy<T, match_if<is_recordset_alias, T>> : std::remove_const<type_t<T>> {};

        template<class T>
        using mapped_type_proxy_t = typename mapped_type_proxy<T>::type;
    }
}
