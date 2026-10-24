#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_const, std::declval
#if __cpp_lib_unwrap_ref >= 201811L
#include <utility>  //  std::reference_wrapper
#else
#include <functional>  //  std::reference_wrapper
#endif
#endif

#include "cxx_polyfill/type_traits.h"

namespace sqlite_orm::internal {
    /**
     *  The element type behind a storage slot of type `T`, always non-reference and
     *  non-const -- akin to `std::iterator_traits<It>::value_type` versus `reference`:
     *  it describes what is stored, not what accessing it hands back.
     *
     *  A `std::reference_wrapper<U>` names `U` with any top-level const on `U` stripped;
     *  everything else is decayed via `remove_cvref`. This intentionally diverges from
     *  `forward_ref`/`unwrap_ref_or_forward_t`, which preserve a wrapped referent's
     *  constness -- the two agree on identity but not necessarily on constness.
     *
     *  Note: the `reference_wrapper` specialization matches only on a bare, unqualified
     *  `std::reference_wrapper<T>`. A `T` that is itself a reference or a cv-qualified
     *  reference_wrapper (as would arise from a forwarding reference or an lvalue member
     *  access) currently falls through to the primary template and stays wrapped. Not
     *  exercised by any caller in this library today.
     */
    template<class T>
    struct value_unref_type : std::remove_const<T> {};

    template<class T>
    struct value_unref_type<std::reference_wrapper<T>> : std::remove_const<T> {};

    template<class T>
    using value_unref_type_t = typename value_unref_type<T>::type;

    /**
     *  Perfectly forward a value, unwrapping a `std::reference_wrapper` on the way.
     *
     *  A `reference_wrapper` argument yields an lvalue reference to the referenced object,
     *  whatever the wrapper's own value category and constness - the referent is
     *  independent of the wrapper that names it. Every other argument is forwarded
     *  unchanged.
     *
     *  This is the value-level facility that `std::unwrap_reference` doesn't provide on
     *  its own: `std::unwrap_reference` unwraps only when its argument type is exactly a
     *  bare `reference_wrapper<U>`, not a reference to one. `value_unref_type` is the
     *  companion that names the resulting object's type instead of a reference to it,
     *  though it always strips constness where this preserves it -- see `value_unref_type`.
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
     *  Unwrap a `std::reference_wrapper` lvalue, or pass any other lvalue through as-is.
     *
     *  Accepts lvalues only, hence always returns an lvalue reference - use it to reach the
     *  object behind a stored member or tuple element that may or may not be a
     *  `reference_wrapper`, without deciding at the call site which of the two it is.
     *  The argument's constness carries over, except through a `reference_wrapper`, whose
     *  referent is unaffected by the constness of the wrapper.
     */
    template<class T>
    decltype(auto) forward_lvalue_ref(T& refd) {
        return forward_ref(refd);
    }

    /**
     *  The reference type `forward_ref()` returns for `T`: `T&` stays `T&`, a bare `T`
     *  becomes `T&&`, and a `reference_wrapper<U>` in any form becomes `U&`.
     *
     *  Where `value_unref_type` answers what element type sits behind a storage type,
     *  this answers what accessing that storage hands out -- the two agree on identity,
     *  but this one preserves a wrapped referent's constness where `value_unref_type`
     *  strips it.
     */
    template<class T>
    struct unwrap_ref_or_forward {
        using type = decltype(forward_ref(std::declval<T>()));
    };

    template<class T>
    using unwrap_ref_or_forward_t = typename unwrap_ref_or_forward<T>::type;
}
