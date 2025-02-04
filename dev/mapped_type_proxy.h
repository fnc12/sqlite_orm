#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_const
#endif

#include "type_traits.h"
#include "table_reference.h"
#include "alias_traits.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  If T is a table reference or recordset alias then the typename mapped_type_proxy<T>::type is the unqualified aliased type,
         *  otherwise unqualified T.
         */
        template<class T, class SFINAE = void>
        struct mapped_type_proxy : std::remove_const<T> {};

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<orm_table_reference R>
        struct mapped_type_proxy<R, void> : R {};
#endif

        template<class A>
        struct mapped_type_proxy<A, match_if<is_recordset_alias, A>> : std::remove_const<type_t<A>> {};

        template<class T>
        using mapped_type_proxy_t = typename mapped_type_proxy<T>::type;
    }
}
