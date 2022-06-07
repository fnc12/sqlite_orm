#pragma once

#include <type_traits>  // std::integral_constant
#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && SQLITE_ORM_HAS_INCLUDE(<concepts>)
#include <concepts>
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    using xdestroy_fn_t = void (*)(void*);
    using null_xdestroy_t = std::integral_constant<xdestroy_fn_t, nullptr>;
    SQLITE_ORM_INLINE_VAR constexpr null_xdestroy_t null_xdestroy_f{};
}

namespace sqlite_orm {
    namespace internal {
#ifdef SQLITE_ORM_CONCEPTS_SUPPORTED
        /**
         *  Constraints a deleter to be state-less.
         */
        template<typename D>
        concept stateless_deleter = std::is_empty_v<D> && std::is_default_constructible_v<D>;

        /**
         *  Constraints a deleter to be an integral function constant.
         */
        template<typename D>
        concept integral_fp_c = requires {
            typename D::value_type;
            D::value;
            requires std::is_function_v<std::remove_pointer_t<typename D::value_type>>;
        };

        /**
         *  Constraints a deleter to be or to yield a function pointer.
         */
        template<typename D>
        concept yields_fp = requires(D d) {
            // yielding function pointer by using the plus trick
            {+d};
            requires std::is_function_v<std::remove_pointer_t<decltype(+d)>>;
        };
#endif

#if __cpp_lib_concepts >= 201907L
        /**
         *  Yield a deleter's function pointer.
         */
        template<yields_fp D>
        struct yield_fp_of {
            using type = decltype(+std::declval<D>());
        };
#else

        template<typename D>
        SQLITE_ORM_INLINE_VAR constexpr bool is_stateless_deleter_v =
            std::is_empty<D>::value&& std::is_default_constructible<D>::value;

        template<typename D, typename SFINAE = void>
        struct is_integral_fp_c : std::false_type {};
        template<typename D>
        struct is_integral_fp_c<
            D,
            polyfill::void_t<typename D::value_type,
                             decltype(D::value),
                             std::enable_if_t<std::is_function<std::remove_pointer_t<typename D::value_type>>::value>>>
            : std::true_type {};
        template<typename D>
        SQLITE_ORM_INLINE_VAR constexpr bool is_integral_fp_c_v = is_integral_fp_c<D>::value;

        template<typename D, typename SFINAE = void>
        struct can_yield_fp : std::false_type {};
        template<typename D>
        struct can_yield_fp<
            D,
            polyfill::void_t<
                decltype(+std::declval<D>()),
                std::enable_if_t<std::is_function<std::remove_pointer_t<decltype(+std::declval<D>())>>::value>>>
            : std::true_type {};
        template<typename D>
        SQLITE_ORM_INLINE_VAR constexpr bool can_yield_fp_v = can_yield_fp<D>::value;

        template<typename D, bool = can_yield_fp_v<D>>
        struct yield_fp_of {
            using type = void;
        };
        template<typename D>
        struct yield_fp_of<D, true> {
            using type = decltype(+std::declval<D>());
        };
#endif
        template<typename D>
        using yielded_fn_t = typename yield_fp_of<D>::type;

#if __cpp_lib_concepts >= 201907L
        template<typename D>
        concept is_unusable_for_xdestroy = (!stateless_deleter<D> &&
                                            (yields_fp<D> && !std::convertible_to<yielded_fn_t<D>, xdestroy_fn_t>));

        /**
         *  This concept tests whether a deleter yields a function pointer, which is convertible to an xdestroy function pointer.
         *  Note: We are using 'is convertible' rather than 'is same' because of any exception specification.
         */
        template<typename D>
        concept yields_xdestroy = yields_fp<D> && std::convertible_to<yielded_fn_t<D>, xdestroy_fn_t>;

        template<typename D, typename P>
        concept needs_xdestroy_proxy = (stateless_deleter<D> &&
                                        (!yields_fp<D> || !std::convertible_to<yielded_fn_t<D>, xdestroy_fn_t>));

        /**
         *  xDestroy function that constructs and invokes the stateless deleter.
         *  
         *  Requires that the deleter can be called with the q-qualified pointer argument;
         *  it doesn't check so explicitly, but a compiler error will occur.
         */
        template<typename D, typename P>
        requires(!integral_fp_c<D>) void xdestroy_proxy(void* p) noexcept {
            // C-casting `void* -> P*` like statement_binder<pointer_binding<P, T, D>>
            auto o = (P*)p;
            // ignoring return code
            (void)D{}(o);
        }

        /**
         *  xDestroy function that invokes the integral function pointer constant.
         *  
         *  Performs a const-cast of the argument pointer in order to allow for C API functions
         *  that take a non-const parameter, but user code passes a pointer to a const object.
         */
        template<integral_fp_c D, typename P>
        void xdestroy_proxy(void* p) noexcept {
            // C-casting `void* -> P*` like statement_binder<pointer_binding<P, T, D>>,
            auto o = (std::remove_cv_t<P>*)(P*)p;
            // ignoring return code
            (void)D{}(o);
        }
#else
        template<typename D>
        SQLITE_ORM_INLINE_VAR constexpr bool is_unusable_for_xdestroy_v =
            !is_stateless_deleter_v<D> &&
            (can_yield_fp_v<D> && !std::is_convertible<yielded_fn_t<D>, xdestroy_fn_t>::value);

        template<typename D>
        SQLITE_ORM_INLINE_VAR constexpr bool can_yield_xdestroy_v =
            can_yield_fp_v<D>&& std::is_convertible<yielded_fn_t<D>, xdestroy_fn_t>::value;

        template<typename D, typename P>
        SQLITE_ORM_INLINE_VAR constexpr bool needs_xdestroy_proxy_v =
            is_stateless_deleter_v<D> &&
            (!can_yield_fp_v<D> || !std::is_convertible<yielded_fn_t<D>, xdestroy_fn_t>::value);

        template<typename D, typename P, std::enable_if_t<!is_integral_fp_c_v<D>, bool> = true>
        void xdestroy_proxy(void* p) noexcept {
            // C-casting `void* -> P*` like statement_binder<pointer_binding<P, T, D>>
            auto o = (P*)p;
            // ignoring return code
            (void)D{}(o);
        }

        template<typename D, typename P, std::enable_if_t<is_integral_fp_c_v<D>, bool> = true>
        void xdestroy_proxy(void* p) noexcept {
            // C-casting `void* -> P*` like statement_binder<pointer_binding<P, T, D>>,
            auto o = (std::remove_cv_t<P>*)(P*)p;
            // ignoring return code
            (void)D{}(o);
        }
#endif
    }
}

namespace sqlite_orm {

#if __cpp_lib_concepts >= 201907L
    /**
     *  Prohibits using a yielded function pointer, which is not of type xdestroy_fn_t.
     *  
     *  Explicitly declared for better error messages.
     */
    template<typename D, typename P>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D, P*) noexcept requires(internal::is_unusable_for_xdestroy<D>) {
        static_assert(polyfill::always_false_v<D>,
                      "A function pointer, which is not of type xdestroy_fn_t, is prohibited.");
        return nullptr;
    }

    /**
     *  Obtains a proxy 'xDestroy' function pointer [of type void(*)(void*)]
     *  for a deleter in a type-safe way.
     *  
     *  The deleter can be one of:
     *         - integral function constant
     *         - state-less (empty) deleter
     *         - non-capturing lambda
     *  
     *  Type-safety is garanteed by checking whether the deleter or yielded function pointer
     *  is invocable with the non-q-qualified pointer value.
     */
    template<typename D, typename P>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D, P*) noexcept requires(internal::needs_xdestroy_proxy<D, P>) {
        return internal::xdestroy_proxy<D, P>;
    }

    /**
     *  Directly obtains a 'xDestroy' function pointer [of type void(*)(void*)]
     *  from a deleter in a type-safe way.
     *  
     *  The deleter can be one of:
     *         - function pointer of type xdestroy_fn_t
     *         - structure holding a function pointer
     *         - integral function constant
     *         - non-capturing lambda
     *  ... and yield a function pointer of type xdestroy_fn_t.
     *  
     *  Type-safety is garanteed by checking whether the deleter or yielded function pointer
     *  is invocable with the non-q-qualified pointer value.
     */
    template<typename D, typename P>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D d, P*) noexcept requires(internal::yields_xdestroy<D>) {
        return d;
    }
#else
    template<typename D, typename P, std::enable_if_t<internal::is_unusable_for_xdestroy_v<D>, bool> = true>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D, P*) {
        static_assert(polyfill::always_false_v<D>,
                      "A function pointer, which is not of type xdestroy_fn_t, is prohibited.");
        return nullptr;
    }

    template<typename D, typename P, std::enable_if_t<internal::needs_xdestroy_proxy_v<D, P>, bool> = true>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D, P*) noexcept {
        return internal::xdestroy_proxy<D, P>;
    }

    template<typename D, typename P, std::enable_if_t<internal::can_yield_xdestroy_v<D>, bool> = true>
    constexpr xdestroy_fn_t obtain_xdestroy_for(D d, P*) noexcept {
        return d;
    }
#endif
}
