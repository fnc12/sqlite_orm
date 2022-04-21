#pragma once

namespace sqlite_orm {
    namespace internal {

        template<class T, class SFINAE = void>
        struct field_member_traits;

        template<class O, class F>
        struct field_member_traits<F O::*, void> {
            using object_type = O;
            using field_type = F;
        };
    }
}
