#pragma once

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
    /**
     *  Stores OFFSET only info
     */
    template<class T>
    struct offset_t {
        T offset;
    };

    template<class T>
    constexpr bool is_offset_v = polyfill::is_specialization_of<T, offset_t>::value;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  OFFSET clause.
     *  Example: offset(5)
     *  @param offset The offset value (number of rows to skip).
     *  @return offset_t instance representing OFFSET clause.
     */
    template<class T>
    internal::offset_t<T> offset(T offset) {
        return {std::move(offset)};
    }
}
