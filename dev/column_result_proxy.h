#pragma once

#include "type_traits.h"
#include "table_reference.h"

namespace sqlite_orm {
    namespace internal {

        /*
         *  Holder for the type of an unmapped aggregate/structure/object to be constructed ad-hoc from column results.
         *  `T` must be constructible using direct-list-initialization.
         */
        template<class T, class ColResults>
        struct structure {
            using type = T;
        };
    }
}

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct column_result_proxy : std::remove_const<T> {};

        /*
         *  Unwrap `table_reference`
         */
        template<class P>
        struct column_result_proxy<P, match_if<is_table_reference, P>> : decay_table_ref<P> {};

        /*
         *  Pass through `structure`
         */
        template<class P>
        struct column_result_proxy<P, match_specialization_of<P, structure>> : P {};

        template<class T>
        using column_result_proxy_t = typename column_result_proxy<T>::type;
    }
}
