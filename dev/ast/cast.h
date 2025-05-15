#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  // std::string
#include <utility>  // std::move
#endif  //  SQLITE_ORM_IMPORT_STD_MODULE
namespace sqlite_orm {

    namespace internal {

        /**
         *  CAST holder.
         *  T is a type to cast to
         *  E is an expression type
         *  Example: cast<std::string>(&User::id)
         */
        template<class T, class E>
        struct cast_t {
            using to_type = T;
            using expression_type = E;

            expression_type expression;
        };
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  CAST(X AS type).
     *  Example: cast<std::string>(&User::id)
     */
    template<class T, class E>
    internal::cast_t<T, E> cast(E e) {
        return {std::move(e)};
    }
}
