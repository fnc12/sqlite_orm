#pragma once

/** @file Declarations of closed predicates on raw C++ types, checking whether they qualify for an SQL-level role.
 */

#include "../../functional/cxx_type_traits_polyfill.h"

// Derived from the type itself, defined in `field_predicates.h`
namespace sqlite_orm::internal {
    template<class F, class SFINAE = void>
    extern const bool is_rowid_alias_capable_v;

    template<class F>
    using is_rowid_alias_capable = std::bool_constant<is_rowid_alias_capable_v<F>>;

    /*
     *  Whether a type is a C++ text value - a narrow or wide C string, string view or string -,
     *  which a select yields as `std::string`.
     */
    template<class T>
    extern const bool is_text_value_v;

    template<class T>
    using is_text_value = std::bool_constant<is_text_value_v<T>>;
}

// Tied to a customization point of the type mapping, defined with it
namespace sqlite_orm::internal {
    /*
     *  Whether a C++ type maps to SQLite's INTEGER type: its `type_printer` derives from `integer_printer`.
     */
    template<class T>
    extern const bool is_integer_mapped_v;

    template<class T>
    using is_integer_mapped = std::bool_constant<is_integer_mapped_v<T>>;
}

// Tied to a customization point of the marshaling, defined with it
namespace sqlite_orm::internal {
    /*
     *  Whether a field type can be bound as a parameter of a prepared statement: `statement_binder` is
     *  instantiable for it.
     */
    template<class T, class SFINAE = void>
    extern const bool is_bindable_v;

    //  a derived struct in favor of an alias template, because it is passed on as a template-template argument
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
    template<class T>
    struct is_bindable : std::bool_constant<is_bindable_v<T>> {};

    /*
     *  Whether a field type can be printed as a human-readable string: `field_printer` is instantiable for it.
     */
    template<class T, class SFINAE = void>
    extern const bool is_printable_v;

    //  a derived struct in favor of an alias template to be on the safe side in case it is passed on as a template-template argument
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR]
    template<class T>
    struct is_printable : std::bool_constant<is_printable_v<T>> {};
}

// Values of the pointer-passing interface, defined in `pointer_value.h`; always false before SQLite 3.20.
// The top-level headers include it ahead of `storage.h`, as the marshaling headers evaluate them non-dependently.
namespace sqlite_orm::internal {
    /*
     *  Whether a type is a `pointer_arg`: a function parameter receiving a pointer value.
     */
    template<class T>
    extern const bool is_pointer_arg_v;

    template<class T>
    using is_pointer_arg = std::bool_constant<is_pointer_arg_v<T>>;

    /*
     *  Whether a type is a `pointer_binding`: a pointer value bound as a parameter or set as a function result.
     */
    template<class T>
    extern const bool is_pointer_binding_v;

    template<class T>
    using is_pointer_binding = std::bool_constant<is_pointer_binding_v<T>>;
}
