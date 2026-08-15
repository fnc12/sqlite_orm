#pragma once

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#include <functional>
#if __cpp_lib_invoke < 201411L
#include <type_traits>  //  std::enable_if, std::is_member_object_pointer, std::is_member_function_pointer
#endif
#include <utility>  //  std::forward
#endif

#include "cxx_type_traits_polyfill.h"
#include "../member_traits/member_traits.h"

namespace sqlite_orm::internal::polyfill {
    // C++20 or later (unfortunately there's no feature test macro).
    // Stupidly, clang says C++20, but `std::identity` was only implemented in libc++ 13 and libstd++-v3 10
    // (the latter is used on Linux).
    // gcc got it right and reports C++20 only starting with v10.
    // The check here doesn't care and checks the library versions in use.
    //
    // Another way of detection would be the constrained algorithms feature-test macro __cpp_lib_ranges
#if (__cplusplus >= 202002L) &&                                                                                        \
    ((!_LIBCPP_VERSION || _LIBCPP_VERSION >= 13000) && (!_GLIBCXX_RELEASE || _GLIBCXX_RELEASE >= 10))
    using std::identity;
#else
    struct identity {
        template<class T>
        constexpr T&& operator()(T&& v) const noexcept {
            return std::forward<T>(v);
        }

        using is_transparent = int;
    };
#endif

#if __cpp_lib_invoke >= 201411L
    using std::invoke;
#else
    // pointer-to-data-member+object
    template<class Callable,
             class Object,
             class... Args,
             class Unqualified = remove_cvref_t<Callable>,
             std::enable_if_t<std::is_member_object_pointer<Unqualified>::value, bool> = true>
    decltype(auto) invoke(Callable&& callable, Object&& object, Args&&... args) {
        return std::forward<Object>(object).*callable;
    }

    // pointer-to-member-function+object
    template<class Callable,
             class Object,
             class... Args,
             class Unqualified = remove_cvref_t<Callable>,
             std::enable_if_t<std::is_member_function_pointer<Unqualified>::value, bool> = true>
    decltype(auto) invoke(Callable&& callable, Object&& object, Args&&... args) {
        return (std::forward<Object>(object).*callable)(std::forward<Args>(args)...);
    }

    // pointer-to-member+reference-wrapped object (expect `reference_wrapper::*`)
    template<
        class Callable,
        class Object,
        class... Args,
        std::enable_if_t<
            polyfill::negation<polyfill::is_specialization_of<member_object_type_t<std::remove_reference_t<Callable>>,
                                                              std::reference_wrapper>>::value,
            bool> = true>
    decltype(auto) invoke(Callable&& callable, std::reference_wrapper<Object> wrapper, Args&&... args) {
        return invoke(std::forward<Callable>(callable), wrapper.get(), std::forward<Args>(args)...);
    }

    // functor
    template<class Callable, class... Args>
    decltype(auto) invoke(Callable&& callable, Args&&... args) {
        return std::forward<Callable>(callable)(std::forward<Args>(args)...);
    }
#endif

#if __cpp_lib_is_invocable >= 201703L
    using std::is_invocable;
#else
    template<class Void, class... X>
    struct is_invocable_impl : std::false_type {};

#if __cplusplus >= 201703L
    template<class... Ts>
    struct is_invocable_impl<polyfill::void_t<decltype(polyfill::invoke(std::declval<Ts>()...))>, Ts...>
        : std::true_type {};
#else
    template<class Callable, class... Args>
    struct is_invocable_impl<
        polyfill::void_t<decltype(std::declval<std::reference_wrapper<std::remove_reference_t<Callable>>>()(
            std::declval<Args>()...))>,
        Callable,
        Args...> : std::true_type {};
#endif

    template<class Callable, class... Args>
    struct is_invocable : is_invocable_impl<void, Callable, Args...>::type {};
#endif

#if __cpp_lib_unwrap_ref >= 201811L
    using std::unwrap_reference, std::unwrap_reference_t, std::unwrap_ref_decay, std::unwrap_ref_decay_t;
#else
    template<class T>
    struct unwrap_reference {
        using type = T;
    };

    template<class T>
    struct unwrap_reference<std::reference_wrapper<T>> {
        using type = T&;
    };

    template<class T>
    using unwrap_reference_t = typename unwrap_reference<T>::type;

    template<class T>
    using unwrap_ref_decay_t = unwrap_reference_t<std::decay_t<T>>;

    template<class T>
    struct unwrap_ref_decay {
        using type = unwrap_ref_decay_t<T>;
    };
#endif
}

namespace sqlite_orm {
    namespace polyfill = internal::polyfill;
}
