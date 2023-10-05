#pragma once

#include <string>  //  std::string
#include <utility>  //  std::move

#include "functional/cxx_core_features.h"
#include "conditions.h"
#include "alias_traits.h"
#include "tags.h"

namespace sqlite_orm {
    namespace internal {
        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer : condition_t {
            using self = column_pointer<T, F>;
            using type = T;
            using field_type = F;

            field_type field;

            column_pointer(field_type field) : field{field} {}
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_pointer_v = polyfill::is_specialization_of_v<T, column_pointer>;

        template<class T>
        using is_column_pointer = polyfill::bool_constant<is_column_pointer_v<T>>;

    }
}
