#pragma once

#include "../functional/cxx_type_traits_polyfill.h"
#include "../vocabulary/traits/structural_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {

    /*
     *  Protect an otherwise bindable element so that it is always serialized as a literal value.
     */
    template<class T>
    struct literal_holder {
        using type = T;

        type value;
    };

    template<class T>
    constexpr bool is_literal_v = polyfill::is_specialization_of_v<T, literal_holder>;
}
