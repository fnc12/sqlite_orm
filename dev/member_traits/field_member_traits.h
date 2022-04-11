#pragma once

#include <type_traits>  //  std::enable_if

#include "is_field_member_pointer.h"

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct field_member_traits;

        template<class O, class F>
        struct field_member_traits<F O::*, std::enable_if_t<is_field_member_pointer<F O::*>::value>> {
            using object_type = O;
            using field_type = F;
        };
    }
}
