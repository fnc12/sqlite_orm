#pragma once

#include <type_traits>  //  std::conditional

#include "../type_traits.h"

namespace sqlite_orm {
    namespace internal {

        /**
         *  Accepts any number of arguments and evaluates `type` alias as T if all arguments are the same or void otherwise
         */
        template<class... Args>
        struct same_or_void {
            using type = void;
        };

        template<class A, class... Rest>
        struct same_or_void<A, Rest...> : std::conditional<is_all_of<A, Rest...>::value, A, void> {};

    }
}
