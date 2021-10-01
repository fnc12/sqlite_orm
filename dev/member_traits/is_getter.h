#pragma once

#include <type_traits>  //  std::false_type, std::true_type

#include "getters.h"

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct is_getter : std::false_type {};

        template<class O, class T>
        struct is_getter<getter_by_value_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_value<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref_const<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref<O, T>> : std::true_type {};
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        struct is_getter<getter_by_value_const_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_value_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref_const_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_ref_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref_const_noexcept<O, T>> : std::true_type {};

        template<class O, class T>
        struct is_getter<getter_by_const_ref_noexcept<O, T>> : std::true_type {};
#endif
    }
}
