#pragma once

#include <utility>  //  std::move

namespace sqlite_orm {
    namespace internal {

        /* 
         *  Protect an otherwise bindable element so that it is always serialized as a literal value.
         */
        template<class T>
        struct literal_holder {
            using type = T;

            T value;
        };

    }
}
