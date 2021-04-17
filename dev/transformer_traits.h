#pragma once

namespace sqlite_orm {

    namespace internal {

        /**
     * This class accepts template function (e.g. call operator) and allows return type extracting 
     */
        template<class T>
        struct transformer_traits;

        template<class O, class A, class R>
        struct transformer_traits<const R &(O::*)(const A &) const> {
            using return_type = R;
            using argument_type = A;
            using transformer_type = O;
        };

    }
}
