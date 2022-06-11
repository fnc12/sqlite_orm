#pragma once

#include <type_traits>  //  std::enable_if, std::is_constructible

#include "fast_and.h"

namespace sqlite_orm {
    namespace internal {
        namespace mpl {

            template<bool SameNumberOfElements, class Tuple, class... Y>
            struct enable_tuple_variadic_ctor {};

            template<class... X, class... Y>
            struct enable_tuple_variadic_ctor<true, tuple<X...>, Y...>
                : std::enable_if<SQLITE_ORM_FAST_AND(std::is_constructible<X, Y&&>), bool> {};
        }
    }

    namespace mpl = mpl;
}
