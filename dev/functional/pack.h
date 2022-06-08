#pragma once

#include "cxx_universal.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            template<typename... T>
            struct pack {
                static constexpr size_t size() {
                    return sizeof...(T);
                }
            };

            template<size_t I, typename T>
            struct indexed_type {
                using type = T;
            };
        }
    }

    namespace mpl = internal::mpl;
}
