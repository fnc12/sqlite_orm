#pragma once

#include <type_traits>

#include "pointer_value.h"

namespace sqlite_orm {

    inline constexpr const char carray_pvt_name[] = "carray";
    using carray_pvt = std::integral_constant<const char*, carray_pvt_name>;
    template<typename P>
    using carray_value = pointer_value<P, carray_pvt>;
    template<typename P, typename D>
    using carray_trule = pointer_trule<P, carray_pvt, D>;

    /**
     *  SQL function that is a pass-through for values
     *  (it returns its argument unchanged using move semantics) but also saves the
     *  value that is passed through into a C-language variable.
     */
    template<typename P>
    struct note_value_fn {
        using pointer_value_t = carray_value<P>;

        P operator()(P&& value, pointer_value_t pv) const {
            if(P* p = pv) {
                *p = value;
            }
            return std::move(value);
        }

        static constexpr const char* name() {
            return "note_value";
        }
    };

    /**
     *  remember(V, $PTR) extension function https://sqlite.org/src/file/ext/misc/remember.c
     */
    struct remember_fn : note_value_fn<int64> {
        static constexpr const char* name() {
            return "remember";
        }
    };
}
