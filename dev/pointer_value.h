#pragma once

#ifndef _IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3020000
#include <type_traits>
#include <memory>
#include <utility>
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>
#endif
#endif
#endif

#include "functional/cstring_literal.h"
#include "xdestroy_handling.h"

#if SQLITE_VERSION_NUMBER >= 3020000
namespace sqlite_orm {
    namespace internal {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<char... C>
        struct pointer_type {
            using value_type = const char[sizeof...(C) + 1];
            static inline constexpr value_type value = {C..., '\0'};
        };
#endif
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    inline namespace literals {
        template<internal::cstring_literal tag>
        [[nodiscard]] consteval auto operator"" _pointer_type() {
            return internal::explode_into<internal::pointer_type, tag>(std::make_index_sequence<tag.size()>{});
        }
    }

    /** @short Specifies that a type is an integral constant string usable as a pointer type.
     */
    template<class T>
    concept orm_pointer_type = requires {
        typename T::value_type;
        { T::value } -> std::convertible_to<const char*>;
    };
#endif

    /**
     *  Wraps a pointer and tags it with a pointer type,
     *  used for accepting function parameters,
     *  facilitating the 'pointer-passing interface'.
     * 
     *  Template parameters:
     *    - P: The value type, possibly const-qualified.
     *    - T: An integral constant string denoting the pointer type, e.g. `"carray"_pointer_type`.
     *
     */
    template<typename P, typename T>
    struct pointer_arg {

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        // note (internal): this is currently a static assertion instead of a type constraint because
        // of forward declarations in other places (e.g. function.h)
        static_assert(orm_pointer_type<T>, "T must be a pointer type (tag)");
#else
        static_assert(std::is_convertible<typename T::value_type, const char*>::value,
                      "The pointer type (tag) must be convertible to `const char*`");
#endif

        using tag = T;
        using qualified_type = P;

        P* p_;

        P* ptr() const noexcept {
            return p_;
        }

        operator P*() const noexcept {
            return p_;
        }
    };

    /**
     *  Pointer value with associated deleter function,
     *  used for returning or binding pointer values
     *  as part of facilitating the 'pointer-passing interface'.
     * 
     *  Template parameters:
     *    - P: The value type, possibly const-qualified.
     *    - T: An integral constant string denoting the pointer type, e.g. `carray_pointer_type`.
     *    - D: The deleter for the pointer value;
     *         can be one of:
     *         - function pointer
     *         - integral function pointer constant
     *         - state-less (empty) deleter
     *         - non-capturing lambda
     *         - structure implicitly yielding a function pointer
     *
     *  @note Use one of the factory functions to create a pointer binding,
     *  e.g. bindable_carray_pointer or statically_bindable_carray_pointer().
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

      protected:
        // Constructing pointer bindings must go through bind_pointer()
        template<class T2, class P2, class D2>
        friend auto bind_pointer(P2*, D2) noexcept -> pointer_binding<P2, T2, D2>;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        // Constructing pointer bindings must go through bind_pointer()
        template<orm_pointer_type auto tag, class P2, class D2>
        friend auto bind_pointer(P2*, D2) noexcept -> pointer_binding<P2, decltype(tag), D2>;
#endif
        template<class B>
        friend B bind_pointer(typename B::qualified_type*, typename B::deleter_type) noexcept;

        // Construct from pointer and deleter.
        // Transfers ownership of the passed in object.
        pointer_binding(P* p, D d = {}) noexcept : p_{p}, d_{std::move(d)} {}

      public:
        using qualified_type = P;
        using tag = T;
        using deleter_type = D;

        pointer_binding(const pointer_binding&) = delete;
        pointer_binding& operator=(const pointer_binding&) = delete;
        pointer_binding& operator=(pointer_binding&&) = delete;

        pointer_binding(pointer_binding&& other) noexcept :
            p_{std::exchange(other.p_, nullptr)}, d_{std::move(other.d_)} {}

        ~pointer_binding() {
            if(p_) {
                if(auto xDestroy = get_xdestroy()) {
                    // note: C-casting `P* -> void*` like statement_binder<pointer_binding<P, T, D>>
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

        xdestroy_fn_t get_xdestroy() const noexcept {
            return obtain_xdestroy_for(d_, p_);
        }
    };

    /**
     *  Alias template for a static pointer value binding.
     *  'Static' means that ownership won't be transferred to sqlite,
     *  sqlite doesn't delete it, and sqlite assumes the object
     *  pointed to is valid throughout the lifetime of a statement.
     */
    template<typename P, typename T>
    using static_pointer_binding = pointer_binding<P, T, null_xdestroy_t>;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<class P, orm_pointer_type auto tag>
    using pointer_arg_t = pointer_arg<P, decltype(tag)>;

    template<class P, orm_pointer_type auto tag, class D>
    using pointer_binding_t = pointer_binding<P, decltype(tag), D>;

    /**
     *  Alias template for a static pointer value binding.
     *  'Static' means that ownership won't be transferred to sqlite,
     *  sqlite doesn't delete it, and sqlite assumes the object
     *  pointed to is valid throughout the lifetime of a statement.
     */
    template<typename P, orm_pointer_type auto tag>
    using static_pointer_binding_t = pointer_binding_t<P, tag, null_xdestroy_t>;
#endif
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    /**
     *  Wrap a pointer, its type and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter when the statement finishes.
     */
    template<class T, class P, class D>
    auto bind_pointer(P* p, D d) noexcept -> pointer_binding<P, T, D> {
        return {p, std::move(d)};
    }

    template<class T, class P, class D>
    auto bind_pointer(std::unique_ptr<P, D> p) noexcept -> pointer_binding<P, T, D> {
        return bind_pointer<T>(p.release(), p.get_deleter());
    }

    template<typename B>
    auto bind_pointer(typename B::qualified_type* p, typename B::deleter_type d = {}) noexcept -> B {
        return B{p, std::move(d)};
    }

    template<class T, class P, class D>
    [[deprecated("Use the better named function `bind_pointer(...)`")]] pointer_binding<P, T, D>
    bindable_pointer(P* p, D d) noexcept {
        return bind_pointer<T>(p, std::move(d));
    }

    template<class T, class P, class D>
    [[deprecated("Use the better named function `bind_pointer(...)`")]] pointer_binding<P, T, D>
    bindable_pointer(std::unique_ptr<P, D> p) noexcept {
        return bind_pointer<T>(p.release(), p.get_deleter());
    }

    template<typename B>
    [[deprecated("Use the better named function `bind_pointer(...)`")]] B
    bindable_pointer(typename B::qualified_type* p, typename B::deleter_type d = {}) noexcept {
        return bind_pointer<B>(p, std::move(d));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Wrap a pointer, its type (tag) and its deleter function for binding it to a statement.
     *  
     *  Unless the deleter yields a nullptr 'xDestroy' function the ownership of the pointed-to-object
     *  is transferred to the pointer binding, which will delete it through
     *  the deleter when the statement finishes.
     */
    template<orm_pointer_type auto tag, class P, class D>
    auto bind_pointer(P* p, D d) noexcept -> pointer_binding<P, decltype(tag), D> {
        return {p, std::move(d)};
    }

    template<orm_pointer_type auto tag, class P, class D>
    auto bind_pointer(std::unique_ptr<P, D> p) noexcept -> pointer_binding<P, decltype(tag), D> {
        return bind_pointer<tag>(p.release(), p.get_deleter());
    }
#endif

    /**
     *  Wrap a pointer and its type for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<class T, class P>
    auto bind_pointer_statically(P* p) noexcept -> static_pointer_binding<P, T> {
        return bind_pointer<T>(p, null_xdestroy_f);
    }

    template<typename B>
    B bind_pointer_statically(typename B::qualified_type* p,
                              typename B::deleter_type* /*exposition*/ = nullptr) noexcept {
        return bind_pointer<B>(p);
    }

    template<class T, class P>
    [[deprecated("Use the better named function `bind_pointer_statically(...)`")]] static_pointer_binding<P, T>
    statically_bindable_pointer(P* p) noexcept {
        return bind_pointer<T>(p, null_xdestroy_f);
    }

    template<typename B>
    [[deprecated("Use the better named function `bind_pointer_statically(...)`")]] B
    statically_bindable_pointer(typename B::qualified_type* p,
                                typename B::deleter_type* /*exposition*/ = nullptr) noexcept {
        return bind_pointer<B>(p);
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Wrap a pointer and its type (tag) for binding it to a statement.
     *  
     *  Note: 'Static' means that ownership of the pointed-to-object won't be transferred
     *  and sqlite assumes the object pointed to is valid throughout the lifetime of a statement.
     */
    template<orm_pointer_type auto tag, class P>
    auto bind_pointer_statically(P* p) noexcept -> static_pointer_binding<P, decltype(tag)> {
        return bind_pointer<tag>(p, null_xdestroy_f);
    }
#endif

    /**
     *  Forward a pointer value from an argument.
     */
    template<class P, class T>
    auto rebind_statically(const pointer_arg<P, T>& pv) noexcept -> static_pointer_binding<P, T> {
        return bind_pointer_statically<T>(pv.ptr());
    }
}
#endif
