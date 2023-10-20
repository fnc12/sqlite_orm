#pragma once

#include <type_traits>  //  std::enable_if
#include <utility>  // std::move

#include "functional/cxx_type_traits_polyfill.h"
#include "tags.h"

namespace sqlite_orm {
    namespace internal {
        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer {
            using type = T;
            using field_type = F;

            field_type field;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_pointer_v = polyfill::is_specialization_of_v<T, column_pointer>;

        template<class T>
        using is_column_pointer = polyfill::bool_constant<is_column_pointer_v<T>>;

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_operator_argument_v<T, std::enable_if_t<is_column_pointer_v<T>>> = true;
    }

    /**
     *  Use it like this:
     *  struct MyType : BaseType { ... };
     *  storage.select(column<MyType>(&BaseType::id));
     */
    template<class Object, class F, internal::satisfies_not<internal::is_recordset_alias, Object> = true>
    constexpr internal::column_pointer<Object, F> column(F field) {
        return {std::move(field)};
    }
}
