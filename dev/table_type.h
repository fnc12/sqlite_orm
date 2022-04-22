#pragma once

namespace sqlite_orm {

    namespace internal {

        template<class T, class F>
        struct column_pointer;

        /**
         *  Trait class used to define table mapped type by setter/getter/member
         *  T - member pointer
         *  `type` is a type which is mapped.
         *  E.g.
         *  -   `table_type<decltype(&User::id)>::type` is `User`
         *  -   `table_type<decltype(&User::getName)>::type` is `User`
         *  -   `table_type<decltype(&User::setName)>::type` is `User`
         */
        template<class T, class SFINAE = void>
        struct table_type;

        template<class O, class F>
        struct table_type<F O::*, void> {
            using type = O;
        };

        template<class T, class F>
        struct table_type<column_pointer<T, F>, void> {
            using type = T;
        };
    }
}
