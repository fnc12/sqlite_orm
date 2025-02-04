#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::false_type, std::true_type
#include <utility>  //  std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../serialize_result_type.h"

namespace sqlite_orm {
    namespace internal {

        struct where_string {
            serialize_result_type serialize() const {
                return "WHERE";
            }
        };

        /**
         *  WHERE argument holder.
         *  C is expression type. Can be any expression like: is_equal_t, is_null_t, exists_t etc
         *  Don't construct it manually. Call `where(...)` function instead.
         */
        template<class C>
        struct where_t : where_string {
            using expression_type = C;

            expression_type expression;

            constexpr where_t(expression_type expression_) : expression(std::move(expression_)) {}
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_where_v = polyfill::is_specialization_of<T, where_t>::value;

        template<class T>
        struct is_where : polyfill::bool_constant<is_where_v<T>> {};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  WHERE clause. Use it to add WHERE conditions wherever you like.
     *  C is expression type. Can be any expression like: is_equal_t, is_null_t, exists_t etc
     *  @example
     *  //  SELECT name
     *  //  FROM letters
     *  //  WHERE id > 3
     *  auto rows = storage.select(&Letter::name, where(greater_than(&Letter::id, 3)));
     */
    template<class C>
    constexpr internal::where_t<C> where(C expression) {
        return {std::move(expression)};
    }
}
