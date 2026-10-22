#pragma once

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#include <functional>  //  std::reference_wrapper
#include <type_traits>  //  std::decay_t
#include <utility>  //  std::forward
#endif

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
