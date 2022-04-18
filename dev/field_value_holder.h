#pragma once

#include "type_traits.h"
#include "member_traits/is_field_member_pointer.h"
#include "member_traits/getter_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class G, class O, satisfies<is_field_member_pointer, G> = true>
        decltype(auto) access_field_value(G m, O&& o) noexcept {
            return static_cast<O&&>(o).*m;
        }

        template<class G, class O, satisfies<is_getter, G> = true>
        decltype(auto) access_field_value(G m, O&& o) {
            return (static_cast<O&&>(o).*m)();
        }
    }
}
