#pragma once

#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct into_t {
            using type = T;
        };

        template<class T>
        using is_into = polyfill::is_specialization_of<T, into_t>;
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    template<class T>
    internal::into_t<T> into() {
        return {};
    }
}
