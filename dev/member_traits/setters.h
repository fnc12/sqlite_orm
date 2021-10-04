#pragma once

namespace sqlite_orm {
    namespace internal {

        template<class O, class T>
        using setter_by_value = void (O::*)(T);

        template<class O, class T>
        using setter_by_ref = void (O::*)(T&);

        template<class O, class T>
        using setter_by_const_ref = void (O::*)(const T&);
#ifdef SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
        template<class O, class T>
        using setter_by_value_noexcept = void (O::*)(T) noexcept;

        template<class O, class T>
        using setter_by_ref_noexcept = void (O::*)(T&) noexcept;

        template<class O, class T>
        using setter_by_const_ref_noexcept = void (O::*)(const T&) noexcept;
#endif
    }
}
