#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_convertible, std::bool_constant
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "member_traits/member_traits.h"

namespace sqlite_orm {
    namespace internal {
        template<class T, class F>
        struct column_pointer;

        /*
         *  This trait can be used to check whether the object type of a member pointer or column pointer matches the target type.
         *  
         *  One use case is the ability to create column reference to an aliased table column of a derived object field without explicitly using a column pointer.
         *  E.g.
         *  regular: `alias_column<alias_d<Derived>>(column<Derived>(&Base::field))`
         *  short:   `alias_column<alias_d<Derived>>(&Base::field)`
         */
        template<class F, class T, class SFINAE = void>
        inline constexpr bool is_field_of_v = false;

        /*
         *  `true` if a pointer-to-member of Base is convertible to a pointer-to-member of Derived.
         */
        template<class O, class Base, class F>
        inline constexpr bool
            is_field_of_v<F Base::*, O, std::enable_if_t<std::is_convertible<F Base::*, F O::*>::value>> = true;

        template<class F, class T>
        inline constexpr bool is_field_of_v<column_pointer<T, F>, T, void> = true;

        template<class F, class T>
        struct is_field_of : polyfill::bool_constant<is_field_of_v<F, T>> {};

        /*
         *  Compare unrelated fields, like from completely different class types or an empty setter.
         */
        template<class L, class R>
        constexpr bool compare_fields(const L&, const R&) {
            return false;
        }

        /*
         *  Compare related fields, either from the same class types or also if the pointer-to-member is of a base class.
         */
        template<class O1,
                 class O2,
                 class F,
                 std::enable_if_t<is_field_of<F O1::*, O2>::value || is_field_of<F O2::*, O1>::value, bool> = true>
        constexpr bool compare_fields(F O1::* lhs, F O2::* rhs) {
            return lhs == rhs;
        }
    }
}
