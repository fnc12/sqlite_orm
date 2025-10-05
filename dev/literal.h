#pragma once

#include "functional/cxx_type_traits_polyfill.h"

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
    using is_literal = polyfill::is_specialization_of<T, literal_holder>;

    template<class T>
    using table_value_t = literal_holder<T>;

    template<class T>
    using is_table_value = is_literal<T>;
}
