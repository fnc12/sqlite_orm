#pragma once

/** @file Declarations of closed predicates for checking the validity of fields of column nodes.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    template<class F, class SFINAE = void>
    extern const bool is_rowid_alias_capable_v;

    template<class F>
    using is_rowid_alias_capable = std::bool_constant<is_rowid_alias_capable_v<F>>;

    /*
     *  Whether a field type can be bound as a parameter of a prepared statement.
     *
     *  In contrast to `is_rowid_alias_capable`, which is a capability derived from the field type itself,
     *  this is tied to whether the `statement_binder` customization point is instantiable for it -
     *  hence its definition stays with `statement_binder` in `statement_binder.h`.
     */
    template<class T, class SFINAE = void>
    extern const bool is_bindable_v;

    //  a derived struct in favor of an alias template, because it is passed on as a template-template argument
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
    template<class T>
    struct is_bindable : std::bool_constant<is_bindable_v<T>> {};

    /*
     *  Whether a field type can be printed as a human-readable string.
     *
     *  Like `is_bindable`, this is tied to whether the `field_printer` customization point is
     *  instantiable for it - hence its definition stays with `field_printer` in `field_printer.h`.
     */
    template<class T, class SFINAE = void>
    extern const bool is_printable_v;

    template<class T>
    struct is_printable : std::bool_constant<is_printable_v<T>> {};
}
