#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_const
#include <utility>  //  std::forward, std::declval, std::reference_wrapper (declaration)
#if __cpp_lib_unwrap_ref < 201811L
#include <functional>  //  std::reference_wrapper
#endif
#endif

#include "cxx_polyfill/type_traits.h"

// unwrapping of `std::reference_wrapper`
namespace sqlite_orm::internal {
    /**
     *  The non-const element type stored in a slot of type `T`:
     *  the referent of a bare `std::reference_wrapper`, otherwise `T` itself.
     */
    template<class T>
    struct value_unref_type : std::remove_const<T> {};

    template<class T>
    struct value_unref_type<std::reference_wrapper<T>> : std::remove_const<T> {};

    template<class T>
    using value_unref_type_t = typename value_unref_type<T>::type;

    /**
     *  Forwards a value, turning a `std::reference_wrapper` of any value category and constness
     *  into an lvalue reference to its referent.
     */
    template<class T>
    decltype(auto) forward_ref(T&& x) {
        if constexpr (polyfill::is_specialization_of_v<polyfill::remove_cvref_t<T>, std::reference_wrapper>) {
            return x.get();
        } else {
            return std::forward<T>(x);
        }
    }

    /**
     *  Unwraps a `std::reference_wrapper` lvalue, or passes any other lvalue through.
     */
    template<class T>
    decltype(auto) forward_lvalue_ref(T& refd) {
        return forward_ref(refd);
    }

    /**
     *  The reference type `forward_ref()` returns for `T`:
     *  `T&` stays `T&`, `T` becomes `T&&`, and a `std::reference_wrapper<U>`, however qualified, becomes `U&`.
     */
    template<class T>
    struct unwrap_ref_or_forward {
        using type = decltype(forward_ref(std::declval<T>()));
    };

    template<class T>
    using unwrap_ref_or_forward_t = typename unwrap_ref_or_forward<T>::type;
}
