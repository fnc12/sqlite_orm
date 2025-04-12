#pragma once

namespace sqlite_orm {

    namespace internal {

        /*  
         *  Function object whose variadic call operator always returns the default constructed value of its template parameter type.
         */
        template<class R>
        struct always_default_of {
            template<class... Args>
            SQLITE_ORM_STATIC_CALLOP constexpr R operator()(Args&&...) SQLITE_ORM_OR_CONST_CALLOP {
                return R();
            }

            using is_transparent = int;
        };

        template<class R>
        constexpr always_default_of<R> always_default{};
    }

}
