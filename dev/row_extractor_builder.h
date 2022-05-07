#pragma once

#include <cstddef>  //  std::nullptr_t

#include "row_extractor.h"
#include "mapped_row_extractor.h"

namespace sqlite_orm {

    namespace internal {

        template<class T>
        row_extractor<T> make_row_extractor(std::nullptr_t) {
            return {};
        }

        template<class T, class I>
        mapped_row_extractor<T, I> make_row_extractor(const I* tableInfo) {
            return {*tableInfo};
        }
    }

}
