#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_const
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "alias_traits.h"

namespace sqlite_orm::internal {
    /** 
     *  Defines the `type` typename to be:
     *  - The unqualified unwrapped table reference type if T is a table reference.
     *  - The unqualified unwrapped table-valued expression type if T is a table-valued expression.
     *  - The unqualified aliased type if T is a recordset alias.
     *  - The enclosing data struct for eponymous virtual tables with hidden columns.
     *  - ... otherwise unqualified T.
     */
    template<class T, class SFINAE = void>
    struct mapped_type_proxy : std::remove_const<T> {};

    template<class T>
    struct mapped_type_proxy<T, polyfill::void_t<typename T::enclosing_type>> {
        using type = enclosing_type_t<T>;
    };

    template<class R>
    struct mapped_type_proxy<R, match_if<is_table_reference, R>> : R {};

    template<class E>
    struct mapped_type_proxy<E, match_if<is_table_valued_expression, E>> : E {};

    template<class A>
    struct mapped_type_proxy<A, match_if<is_recordset_alias, A>> : std::remove_const<type_t<A>> {};

    template<class T>
    using mapped_type_proxy_t = typename mapped_type_proxy<T>::type;
}
