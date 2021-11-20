#pragma once

#include <type_traits>

#include "start_macros.h"
#include "xdestroy_handling.h"

namespace sqlite_orm {

    /**
     *  Wraps a pointer and tags it with a pointer type,
     *  used for accepting function parameters,
     *  facilitating the 'pointer-passing interface'.
     * 
     *  Template parameters:
     *    - P: The value type, possibly const-qualified.
     *    - T: An integral constant string denoting the pointer type, e.g. carray_pvt_name.
     *
     */
    template<typename P, typename T>
    struct pointer_arg {

        static_assert(std::is_convertible<typename T::value_type, const char*>::value,
                      "`std::integral_constant<>` must be convertible to `const char*`");

        using tag = T;
        P* ptr = nullptr;

        constexpr operator P*() const {
            return ptr;
        }
    };

    /**
     *  Pointer value with associated deleter function,
     *  used for returning or binding pointer values
     *  as part of facilitating the 'pointer-passing interface'.
     * 
     *  Template parameters:
     *    - D: The deleter for the pointer value;
     *         can be one of:
     *         - function pointer
     *         - integral function pointer constant
     *         - state-less (empty) deleter
     *         - non-capturing lambda
     *         - structure implicitly yielding a function pointer
     *
     *  @example
     *  ```
     *  int64 rememberedId;
     *  storage.select(func<remember_fn>(&Object::id, statically_bindable_carray_pointer(&rememberedId)));
     *  ```
     */
    template<typename P, typename T, typename D>
    struct pointer_binding : pointer_arg<P, T> {

        SQLITE_ORM_NOUNIQUEADDRESS
        D d_{};

#if __cplusplus < 201703L  // use of C++14
        pointer_binding(P* p, D d = D{}) : pointer_arg<P, T>{p}, d_{d} {}
#endif

        // constraint: xDestroy function must be invocable: D(P*)
        constexpr xdestroy_fn_t get_deleter() const noexcept {
            return obtain_xdestroy_for(d_, this->ptr);
        }
    };

    /**
     *  Template alias for a static pointer value binding.
     *  'Static' means that ownership won't be transferred to sqlite,
     *  sqlite doesn't delete it, and sqlite assumes the object
     *  pointed to is valid throughout the lifetime of a statement.
     */
    template<typename P, typename T>
    using static_pointer_binding = pointer_binding<P, T, null_xdestroy_t>;
}

namespace sqlite_orm {

    /**
     *  Wrap a pointer, its type and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter yields a nullptr function the ownership of the pointed-to-object
     *  will be transferred to sqlite, which will delete it through
     *  the deleter function when the statement finishes.
     */
    template<class T, class P, class D>
    auto bindable_pointer(P* p, D d) -> pointer_binding<P, T, D> {
        return {p, d};
    }

    template<const char* N, class P, class D>
    auto bindable_pointer(P* p, D d) -> pointer_binding<P, std::integral_constant<const char*, N>, D> {
        return {p, d};
    }

    /**
     *  Wrap a pointer and its type for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred to
     *  sqlite and sqlite assumes the object pointed to is valid throughout
     *  the lifetime of a statement.
     */
    template<class T, class P>
    auto statically_bindable_pointer(P* p) -> static_pointer_binding<P, T> {
        return {p};
    }

    template<const char* N, class P>
    auto statically_bindable_pointer(P* p) -> static_pointer_binding<P, std::integral_constant<const char*, N>> {
        return {p};
    }
}
