#pragma once

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
        struct table_type_of<indexed_column_t<C>> {
            using type = typename table_type_of<C>::type;
        };

        template<class T>
        using table_type_of_t = typename table_type_of<T>::type;
    }
}
