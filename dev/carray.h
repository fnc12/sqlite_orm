#pragma once

/*
 *  Note: This feature needs constexpr variables with external linkage.
 *  which can be achieved before C++17's inline variables, but differs from compiler to compiler.
 *  Hence we make it only available for compilers supporting inline variables.
 */

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#include <utility>  //  std::move
#ifndef SQLITE_ORM_WITH_CPP20_ALIASES
#include <type_traits>  //  std::integral_constant
#endif
#endif

#include "pointer_value.h"

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
namespace sqlite_orm {

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline constexpr orm_pointer_type auto carray_pointer_tag = "carray"_pointer_type;
    // [Deprecation notice] This type is deprecated and will be removed in v1.10. Use the inline variable `carray_pointer_tag` instead.
    using carray_pvt [[deprecated]] = decltype("carray"_pointer_type);

    template<typename P>
    using carray_pointer_arg = pointer_arg_t<P, carray_pointer_tag>;
    template<typename P, typename D>
    using carray_pointer_binding = pointer_binding_t<P, carray_pointer_tag, D>;
    template<typename P>
    using static_carray_pointer_binding = static_pointer_binding_t<P, carray_pointer_tag>;

    /**
     *  Wrap a pointer of type 'carray' and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter when the statement finishes.
     */
    template<class P, class D>
    auto bindable_carray_pointer(P* p, D d) noexcept -> pointer_binding_t<P, carray_pointer_tag, D> {
        return bindable_pointer<carray_pointer_tag>(p, std::move(d));
    }

    /**
     *  Wrap a pointer of type 'carray' for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<class P>
    auto statically_bindable_carray_pointer(P* p) noexcept -> static_pointer_binding_t<P, carray_pointer_tag> {
        return statically_bindable_pointer<carray_pointer_tag>(p);
    }
#else
    inline constexpr const char carray_pointer_name[] = "carray";
    using carray_pointer_type = std::integral_constant<const char*, carray_pointer_name>;
    // [Deprecation notice] This type is deprecated and will be removed in v1.10. Use the alias type `carray_pointer_type` instead.
    using carray_pvt [[deprecated]] = carray_pointer_type;

    template<typename P>
    using carray_pointer_arg = pointer_arg<P, carray_pointer_type>;
    template<typename P, typename D>
    using carray_pointer_binding = pointer_binding<P, carray_pointer_type, D>;
    template<typename P>
    using static_carray_pointer_binding = static_pointer_binding<P, carray_pointer_type>;

    /**
     *  Wrap a pointer of type 'carray' and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter when the statement finishes.
     */
    template<class P, class D>
    auto bindable_carray_pointer(P* p, D d) noexcept -> pointer_binding<P, carray_pointer_type, D> {
        return bindable_pointer<carray_pointer_type>(p, std::move(d));
    }

    /**
     *  Wrap a pointer of type 'carray' for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<class P>
    auto statically_bindable_carray_pointer(P* p) noexcept -> static_pointer_binding<P, carray_pointer_type> {
        return statically_bindable_pointer<carray_pointer_type>(p);
    }
#endif

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
