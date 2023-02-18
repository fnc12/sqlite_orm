#pragma once

/*
 *  Note: This feature needs constexpr variables with external linkage.
 *  which can be achieved before C++17's inline variables, but differs from compiler to compiler.
 *  Hence we make it only available for compilers supporting inline variables.
 */

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#include <type_traits>  //  std::integral_constant
#include <utility>  //  std::move

#include "functional/cxx_universal.h"
#include "pointer_value.h"

namespace sqlite_orm {

    inline constexpr const char carray_pvt_name[] = "carray";
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
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter when the statement finishes.
     */
    template<class P, class D>
    auto bindable_carray_pointer(P* p, D d) noexcept -> pointer_binding<P, carray_pvt, D> {
        return bindable_pointer<carray_pvt>(p, std::move(d));
    }

    /**
     *  Wrap a pointer of type 'carray' for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<class P>
    auto statically_bindable_carray_pointer(P* p) noexcept -> static_pointer_binding<P, carray_pvt> {
        return statically_bindable_pointer<carray_pvt>(p);
    }

    /**
     *  Generalized form of the 'remember' SQL function that is a pass-through for values
     *  (it returns its argument unchanged using move semantics) but also saves the
     *  value that is passed through into a bound variable.
     */
    template<typename P>
    struct note_value_fn {
        P operator()(P&& value, carray_pointer_arg<P> pv) const {
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
#endif
