#pragma once

#include <type_traits>
#ifdef __cpp_lib_concepts
#include <concepts>
#endif

#include "start_macros.h"

namespace sqlite_orm {

    namespace internal {

        template<typename T>
        struct fp_type {
            using type = typename T::value_type;
        };
        template<typename R, typename... Args>
        struct fp_type<R (*)(Args...)> {
            using type = R (*)(Args...);
        };
        template<typename T>
        using fp_type_t = typename fp_type<T>::type;
    }

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

        static_assert(std::is_convertible_v<T::value_type, const char*>,
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
     *    - D: The deleter function for the pointer value;
     *         must be either a function pointer type,
     *         integral constant or structure returning a function pointer.
     *
     *  @example
     *  ```
     *  int64 rememberedId;
     *  storage.select(func<remember_fn>(&Object::id, statically_bindable_carray_pointer(&rememberedId)));
     *  ```
     */
#ifdef __cpp_lib_concepts
    template<typename P, typename T, typename D>
    struct pointer_binding : pointer_arg<P, T> {

        SQLITE_ORM_NOUNIQUEADDRESS
        D d_{};

        using fn_t = internal::fp_type_t<D>;

        // constraint: xDestroy function must be invocable: D(P*)
        constexpr xdestroy_fn_t get_deleter() const requires std::invocable < fn_t, std::remove_cv_t<P>
        * > {
            fn_t destroy = d_;
            return xdestroy_fn_t(void_fn_t(destroy));
        }
    };
#elif __cplusplus >= 201703L  // use of C++17 or higher
    template<typename P, typename T, typename D>
    struct pointer_binding : pointer_arg<P, T> {

        D d_{};

        using fn_t = internal::fp_type_t<D>;

        // constraint: xDestroy function must be invocable: D(P*)
        constexpr std::enable_if_t<std::is_invocable_v<fn_t, std::remove_cv_t<P>*>, xdestroy_fn_t> get_deleter() const {
            fn_t destroy = d_;
            return xdestroy_fn_t(void_fn_t(destroy));
        }
    };
#else
    template<typename P, typename T, typename D>
    struct pointer_binding : pointer_arg<P, T> {

        D d_;

        pointer_binding(P* p, D d = D{}) : pointer_arg<P, T>{p}, d_{d} {}

        // constraint: xDestroy function must be void(*)(void*)
        constexpr xdestroy_fn_t get_deleter() const {
            xdestroy_fn_t destroy = d_;
            return destroy;
        }
    };
#endif

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
     *  Unless the deleter carries a nullptr function the ownership
     *  will be transferred to sqlite, which will delete it through
     *  the the deleter function when the statement finishes.
     */
    template<class T, class P, class D>
    auto bindable_pointer(P* p, D d) -> pointer_binding<P, T, D> {
        return {p, d};
    }

    template<const char* I, class P, class D>
    auto bindable_pointer(P* p, D d) -> pointer_binding<P, std::integral_constant<const char*, I>, D> {
        return {p, d};
    }

    /**
     *  Wrap a pointer and its type for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership won't be transferred to
     *  sqlite and sqlite assumes the object pointed to is valid throughout
     *  the lifetime of a statement.
     */
    template<class T, class P>
    auto statically_bindable_pointer(P* p) -> static_pointer_binding<P, T> {
        return {p};
    }

    template<const char* I, class P>
    auto statically_bindable_pointer(P* p) -> static_pointer_binding<P, std::integral_constant<const char*, I>> {
        return {p};
    }
}
