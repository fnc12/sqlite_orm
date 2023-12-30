#pragma once

#include <type_traits>  //  std::enable_if
#include <utility>  // std::move

#include "functional/cxx_type_traits_polyfill.h"
#include "tags.h"
#include "alias_traits.h"

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
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_pointer_v =
            polyfill::is_specialization_of<T, column_pointer>::value;

        template<class T>
        struct is_column_pointer : polyfill::bool_constant<is_column_pointer_v<T>> {};

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_operator_argument_v<T, std::enable_if_t<is_column_pointer<T>::value>> =
            true;
    }

    /**
     *  Use it like this:
     *  struct MyType : BaseType { ... };
     *  storage.select(column<MyType>(&BaseType::id));
     */
    template<class Object, class F, class O, internal::satisfies_not<internal::is_recordset_alias, Object> = true>
    constexpr internal::column_pointer<Object, F O::*> column(F O::*field) {
        static_assert(internal::is_field_of_v<F O::*, Object>, "Column must be from derived class");
        return {field};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Explicitly refer to a column.
     */
    template<orm_table_reference auto table, class O, class F>
    constexpr auto column(F O::*field) {
        return column<internal::auto_type_t<table>>(field);
    }

    // Intentionally place pointer-to-member operator for table references in the internal namespace
    // to facilitate ADL (Argument Dependent Lookup)
    namespace internal {
        /**
         *  Explicitly refer to a column.
         */
        template<orm_table_reference R, class O, class F>
        constexpr auto operator->*(const R& /*table*/, F O::*field) {
            return column<typename R::type>(field);
        }
    }

    /**
     *  Make table reference.
     */
    template<class O>
        requires(!orm_recordset_alias<O>)
    consteval internal::table_reference<O> column() {
        return {};
    }

    /**
     *  Make table reference.
     */
    template<class O>
        requires(!orm_recordset_alias<O>)
    consteval internal::table_reference<O> c() {
        return {};
    }
#endif
}
