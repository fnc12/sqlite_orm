#pragma once

namespace sqlite_orm {
    namespace internal {

        template<class O, class T>
        using getter_by_value_const = T (O::*)() const;

        template<class O, class T>
        using getter_by_value = T (O::*)();

        template<class O, class T>
        using getter_by_ref_const = T& (O::*)() const;

        template<class O, class T>
        using getter_by_ref = T& (O::*)();

        template<class O, class T>
        using getter_by_const_ref_const = const T& (O::*)() const;

        template<class O, class T>
        using getter_by_const_ref = const T& (O::*)();
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        using getter_by_value_const_noexcept = T (O::*)() const noexcept;

        template<class O, class T>
        using getter_by_value_noexcept = T (O::*)() noexcept;

        template<class O, class T>
        using getter_by_ref_const_noexcept = T& (O::*)() const noexcept;

        template<class O, class T>
        using getter_by_ref_noexcept = T& (O::*)() noexcept;

        template<class O, class T>
        using getter_by_const_ref_const_noexcept = const T& (O::*)() const noexcept;

        template<class O, class T>
        using getter_by_const_ref_noexcept = const T& (O::*)() noexcept;
#endif
    }
}
