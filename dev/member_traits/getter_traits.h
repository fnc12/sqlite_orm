#pragma once

#include "getters.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct getter_traits;

        template<class O, class T>
        struct getter_traits<getter_by_value_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_value<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref_const<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        struct getter_traits<getter_by_value_const_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_value_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = false;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref_const_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_ref_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref_const_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };

        template<class O, class T>
        struct getter_traits<getter_by_const_ref_noexcept<O, T>> {
            using object_type = O;
            using field_type = T;

            static constexpr const bool returns_lvalue = true;
        };
#endif
    }
}
