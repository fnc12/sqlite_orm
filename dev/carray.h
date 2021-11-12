#pragma once

#include <type_traits>

#include "start_macros.h"
#include "pointer_value.h"

namespace sqlite_orm {

    SQLITE_ORM_INLINE_VAR constexpr const char carray_pvt_name[] = "carray";
    using carray_pvt = std::integral_constant<const char*, carray_pvt_name>;

    template<typename P>
    using carray_pointer_arg = pointer_arg<P, carray_pvt>;
    template<typename P, typename D>
    using carray_pointer_binding = pointer_binding<P, carray_pvt, D>;
    template<typename P>
    using static_carray_pointer_binding = static_pointer_binding<P, carray_pvt>;

    /**
     *  Wrap a pointer of type 'carray' and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter carries a nullptr function the ownership
     *  will be transferred to sqlite, which will delete it through
     *  the the deleter function when the statement finishes.
     */
    template<class P, class D>
    auto bindable_carray_pointer(P* p, D d) -> pointer_binding<P, carray_pvt, D> {
        return bindable_pointer<carray_pvt>(p, d);
    }

    /**
     *  Wrap a pointer of type 'carray' for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership won't be transferred to
     *  sqlite and sqlite assumes the object pointed to is valid throughout
     *  the lifetime of a statement.
     */
    template<class P>
    auto statically_bindable_carray_pointer(P* p) -> static_pointer_binding<P, carray_pvt> {
        return statically_bindable_pointer<carray_pvt>(p);
    }

    /**
     *  Generalized form of the 'remember' SQL function that is a pass-through for values
     *  (it returns its argument unchanged using move semantics) but also saves the
     *  value that is passed through into a bound variable.
     */
    template<typename P>
    struct note_value_fn {
        using pointer_arg_t = carray_pointer_arg<P>;

        P operator()(P&& value, pointer_arg_t pv) const {
            if(P* observer = pv) {
                *observer = value;
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
