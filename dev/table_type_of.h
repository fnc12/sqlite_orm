#pragma once
#include <type_traits>  //  std::declval
#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    namespace internal {

        template<class T, class F>
        struct column_pointer;

        template<class C>
        struct indexed_column_t;

        /**
         *  Trait class used to define table mapped type by setter/getter/member
         *  T - member pointer
         *  `type` is a type which is mapped.
         *  E.g.
         *  -   `table_type_of<decltype(&User::id)>::type` is `User`
         *  -   `table_type_of<decltype(&User::getName)>::type` is `User`
         *  -   `table_type_of<decltype(&User::setName)>::type` is `User`
         *  -   `table_type_of<decltype(column<User>(&User::id))>::type` is `User`
         *  -   `table_type_of<decltype(derived->*&User::id)>::type` is `User`
         */
        template<class T>
        struct table_type_of;

        template<class O, class F>
        struct table_type_of<F O::*> {
            using type = O;
        };

        template<class T, class F>
        struct table_type_of<column_pointer<T, F>> {
            using type = T;
        };

        template<class C>
        struct table_type_of<indexed_column_t<C>> : table_type_of<C> {};

        template<class T>
        using table_type_of_t = typename table_type_of<T>::type;

        /*
         *  This trait can be used to check whether the object type of a member pointer or column pointer matches the target type.
         *  
         *  One use case is the ability to create column reference to an aliased table column of a derived object field without explicitly using a column pointer.
         *  E.g.
         *  regular: `alias_column<alias_d<Derived>>(column<Derived>(&Base::field))`
         *  short:   `alias_column<alias_d<Derived>>(&Base::field)`
         */
        template<class F, class T, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_field_of_v = false;

        /*
         *  Implementation note: the technique of indirect expression testing is because
         *  of older compilers having problems with the detection of dependent templates [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE].
         */
        template<class FieldOf>
        struct indirectly_test_field_of;

        /*
         *  `true` if a pointer-to-member operator is a valid expression for an object of type `T` and a member pointer of type `F O::*`.
         */
        template<class F, class O, class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_field_of_v<
            F O::*,
            T,
            polyfill::void_t<indirectly_test_field_of<decltype(std::declval<T>().*std::declval<F O::*>())>>> = true;

        template<class F, class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_field_of_v<column_pointer<T, F>, T, void> = true;
    }
}
