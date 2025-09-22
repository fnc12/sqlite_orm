#pragma once

#include "functional/cxx_type_traits_polyfill.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Helper class used for binding fields to sqlite3 statements.
     */
    template<class V, typename SFINAE = void>
    struct statement_binder;
}

namespace sqlite_orm::internal {
    /*
     *  Implementation note: the technique of indirect expression testing is because
     *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
     *  It must also be a type that differs from those for `is_printable_v`, `is_preparable_v`.
     */
    template<class Binder>
    struct indirectly_test_bindable;

    template<class T, class SFINAE = void>
    inline constexpr bool is_bindable_v = false;
    template<class T>
    inline constexpr bool
        is_bindable_v<T, polyfill::void_t<indirectly_test_bindable<decltype(statement_binder<T>{})>>> = true;

    template<class T>
    struct is_bindable : polyfill::bool_constant<is_bindable_v<T>> {};
}
