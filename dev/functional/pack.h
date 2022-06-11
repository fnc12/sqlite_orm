#pragma once

#include <type_traits>  //  std::integral_constant

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

// retain stl tuple interface for `tuple`
namespace std {
    template<class Tpl>
    struct tuple_size;

    template<class... X>
    struct tuple_size<sqlite_orm::mpl::pack<X...>> : integral_constant<size_t, sizeof...(X)> {};
}
