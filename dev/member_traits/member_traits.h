#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_function, std::true_type, std::false_type
#endif

#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm::internal {
    // SFINAE friendly trait to get a member object pointer's field type
    template<class T>
    struct object_field_type {};

    template<class T>
    using object_field_type_t = typename object_field_type<T>::type;

    template<class F, class O>
    struct object_field_type<F O::*> : std::enable_if<!std::is_function<F>::value, F> {};

    // SFINAE friendly trait to get a member function pointer's field type (i.e. unqualified return type)
    template<class T>
    struct getter_field_type {};

    template<class T>
    using getter_field_type_t = typename getter_field_type<T>::type;

    template<class T, class O>
    struct getter_field_type<T O::*> : getter_field_type<T> {};

    template<class F>
    struct getter_field_type<F(void) const> : polyfill::remove_cvref<F> {};

    template<class F>
    struct getter_field_type<F(void)> : polyfill::remove_cvref<F> {};

    template<class F>
    struct getter_field_type<F(void) const noexcept> : polyfill::remove_cvref<F> {};

    template<class F>
    struct getter_field_type<F(void) noexcept> : polyfill::remove_cvref<F> {};

    // SFINAE friendly trait to get a member function pointer's field type (i.e. unqualified parameter type)
    template<class T>
    struct setter_field_type {};

    template<class T>
    using setter_field_type_t = typename setter_field_type<T>::type;

    template<class T, class O>
    struct setter_field_type<T O::*> : setter_field_type<T> {};

    template<class F>
    struct setter_field_type<void(F)> : polyfill::remove_cvref<F> {};

    template<class F>
    struct setter_field_type<void(F) noexcept> : polyfill::remove_cvref<F> {};

    template<class T, class SFINAE = void>
    struct is_getter : std::false_type {};
    template<class T>
    struct is_getter<T, std::void_t<getter_field_type_t<T>>> : std::true_type {};

    template<class T>
    inline constexpr bool is_getter_v = is_getter<T>::value;

    template<class T, class SFINAE = void>
    struct is_setter : std::false_type {};
    template<class T>
    struct is_setter<T, std::void_t<setter_field_type_t<T>>> : std::true_type {};

    template<class T>
    inline constexpr bool is_setter_v = is_setter<T>::value;

    template<class T>
    struct member_field_type : object_field_type<T>, getter_field_type<T>, setter_field_type<T> {};

    template<class T>
    using member_field_type_t = typename member_field_type<T>::type;

    template<class T>
    struct member_object_type {};

    template<class F, class O>
    struct member_object_type<F O::*> : polyfill::type_identity<O> {};

    template<class T>
    using member_object_type_t = typename member_object_type<T>::type;

    /*
     *  Casts the class type of a pointer-to-member from a base class to the specified derived class.
     */
    template<class O, class F, class Base>
    constexpr F O::* as_field_of(F Base::* f) {
        return f;
    }

    /*
     *  Metafunction that casts the class type of a pointer-to-member from a base class to the specified derived class.
     *  note (implementation): go through `member_field_type_t<>` instead of `decltype(as_field_of())` because of
     *  older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
     */
    template<class O, class F>
    using as_field_of_t = member_field_type_t<F> O::*;
}
