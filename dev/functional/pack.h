#pragma once

#include "cxx_universal.h"

namespace _sqlite_orm {
    // short names defined in a short namespace to reduce symbol lengths,
    // since those types are used as a building block;
    // (seen in boost hana)

    template<size_t I, typename T>
    struct indexed_type {
        using type = T;
    };
}

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            using _sqlite_orm::indexed_type;

            template<size_t I, typename T>
            indexed_type<I, T> get_indexed_type(const indexed_type<I, T>&);

            template<typename... T>
            struct pack {
                static constexpr size_t size() {
                    return sizeof...(T);
                }
            };
        }
    }

    namespace mpl = internal::mpl;
}
