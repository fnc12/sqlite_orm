#pragma once

#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    namespace internal {
        /**
         *  Stores OFFSET only info
         */
        template<class T>
        struct offset_t {
            T offset;
        };

        template<class T>
        using is_offset = polyfill::is_specialization_of<T, offset_t>;
    }

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
