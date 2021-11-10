#pragma once

#include <type_traits>
#ifdef __cpp_lib_concepts
#include <concepts>
#endif

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
    struct pointer_value {

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
     *  The term "trule" is the short form of "TRansport capsULE" and is a carrier
     *  object used for moving wrapped types from one place to another.
     *
     *  Template parameters:
     *    - D: The deleter function for the pointer value;
     *         must be either a function pointer type,
     *         integral constant or structure returning a function pointer.
     *
     *  @example
     *  ```
     *  int64 rememberedId;
     *  storage.select(func<remember_fn>(&Object::id, carray_trule<int64, null_xdetroy>{&rememberedId}));
     *  ```
     */
#ifdef __cpp_lib_concepts
    template<typename P, typename T, typename D>
    struct pointer_trule : pointer_value<P, T> {

        SQLITE_ORM_NOUNIQUEADDRESS
        D d_{};

        using fn_t = internal::fp_type_t<D>;

        // constraint: xDestroy function must be invocable: D(P*)
        constexpr xdestroy_fn_t get_deleter() const
            requires std::invocable<fn_t, std::remove_cv_t<P>*>
        {
            fn_t destroy = d_;
            return xdestroy_fn_t(void_fn_t(destroy));
        }
    };
#else
    template<typename P, typename T, typename D>
    struct pointer_trule : pointer_value<P, T> {

        D d_{};

        using fn_t = internal::fp_type_t<D>;

        // constraint: xDestroy function must be invocable: D(P*)
        constexpr std::enable_if_t<
            std::is_invocable_v<fn_t, std::remove_cv_t<P>*>,
            xdestroy_fn_t
        > get_deleter() const {
            fn_t destroy = d_;
            return xdestroy_fn_t(void_fn_t(destroy));
        }
    };
#endif
}
