#pragma once

#include <type_traits>
#include <memory>
#include <utility>

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
        P* p_;

        P* ptr() const noexcept {
            return this->p_;
        }

        constexpr operator P*() const noexcept {
            return p_;
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
    class pointer_binding {

        P* p_;
        SQLITE_ORM_NOUNIQUEADDRESS
        D d_;

      public:
        using tag = T;

        pointer_binding(const pointer_binding&) = delete;
        pointer_binding& operator=(const pointer_binding&) = delete;
        pointer_binding& operator=(pointer_binding&&) = delete;

        // Construct from pointer and deleter.
        // Transfers ownership of the passed in object.
        // Prefer using the factory functions as a simpler public interface.
        pointer_binding(P* p, D d = D{}) noexcept : p_{p}, d_{std::move(d)} {}

        pointer_binding(pointer_binding&& other) noexcept :
            p_{std::exchange(other.p_, nullptr)}, d_{std::move(other.d_)} {}

        ~pointer_binding() {
            if(p_) {
                if(auto xDestroy = get_xdestroy()) {
                    // note: C-casting `P* -> void*` like statement_binder<pointer_binding<P, T, D>
                    xDestroy((void*)p_);
                }
            }
        }

        P* ptr() const noexcept {
            return p_;
        }

        P* take_ptr() noexcept {
            return std::exchange(p_, nullptr);
        }

        constexpr xdestroy_fn_t get_xdestroy() const noexcept {
            return obtain_xdestroy_for(d_, p_);
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
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter function when the statement finishes.
     */
    template<class T, class P, class D>
    auto bindable_pointer(P* p, D d) noexcept -> pointer_binding<P, T, D> {
        return {p, d};
    }

    template<const char* N, class P, class D>
    auto bindable_pointer(P* p, D d) noexcept -> pointer_binding<P, std::integral_constant<const char*, N>, D> {
        return {p, d};
    }

    template<class T, class P, class D>
    auto bindable_pointer(std::unique_ptr<P, D> p) noexcept -> pointer_binding<P, T, D> {
        return {p.release(), p.get_deleter()};
    }

    template<const char* N, class P, class D>
    auto bindable_pointer(std::unique_ptr<P, D> p) noexcept
        -> pointer_binding<P, std::integral_constant<const char*, N>, D> {
        return {p.release(), p.get_deleter()};
    }

    /**
     *  Wrap a pointer and its type for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<class T, class P>
    auto statically_bindable_pointer(P* p) noexcept -> static_pointer_binding<P, T> {
        return {p};
    }

    template<const char* N, class P>
    auto statically_bindable_pointer(P* p) noexcept
        -> static_pointer_binding<P, std::integral_constant<const char*, N>> {
        return {p};
    }
}
