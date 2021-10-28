#pragma once

#include "setters.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct is_setter : std::false_type {};

        template<class O, class T>
        struct is_setter<setter_by_value<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_ref<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_const_ref<O, T>> : std::true_type {};
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        struct is_setter<setter_by_value_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_ref_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_setter<setter_by_const_ref_noexcept<O, T>> : std::true_type {};
#endif
    }
}
