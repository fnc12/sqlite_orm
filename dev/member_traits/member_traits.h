#pragma once

#include <type_traits>  //  std::enable_if, std::is_function, std::true_type, std::false_type

#include "../functional/cxx_universal.h"
#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    namespace internal {
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

#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class F>
        struct getter_field_type<F(void) const noexcept> : polyfill::remove_cvref<F> {};

        template<class F>
        struct getter_field_type<F(void) noexcept> : polyfill::remove_cvref<F> {};
#endif

        // SFINAE friendly trait to get a member function pointer's field type (i.e. unqualified parameter type)
        template<class T>
        struct setter_field_type {};

        template<class T>
        using setter_field_type_t = typename setter_field_type<T>::type;

        template<class T, class O>
        struct setter_field_type<T O::*> : setter_field_type<T> {};

        template<class F>
        struct setter_field_type<void(F)> : polyfill::remove_cvref<F> {};

#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class F>
        struct setter_field_type<void(F) noexcept> : polyfill::remove_cvref<F> {};
#endif

        template<class T, class SFINAE = void>
        struct is_getter : std::false_type {};
        template<class T>
        struct is_getter<T, polyfill::void_t<getter_field_type_t<T>>> : std::true_type {};

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_getter_v = is_getter<T>::value;

        template<class T, class SFINAE = void>
        struct is_setter : std::false_type {};
        template<class T>
        struct is_setter<T, polyfill::void_t<setter_field_type_t<T>>> : std::true_type {};

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_setter_v = is_setter<T>::value;

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
    }
}
