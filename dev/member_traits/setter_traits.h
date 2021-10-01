#pragma once

#include "setters.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct setter_traits;

        template<class O, class T>
        struct setter_traits<setter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;
        };
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        struct setter_traits<setter_by_value_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_ref_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;
        };

        template<class O, class T>
        struct setter_traits<setter_by_const_ref_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;
        };
#endif
    }
}
