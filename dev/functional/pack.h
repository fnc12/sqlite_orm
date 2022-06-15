#pragma once

#include <type_traits>  //  std::integral_constant

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
