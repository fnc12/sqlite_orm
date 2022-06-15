#pragma once

#include "cxx_universal.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (as seen in boost hana)

    template<size_t I, typename T>
    struct indexed_type {
        using type = T;
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            using _sqlite_orm::indexed_type;
        }
    }
}
