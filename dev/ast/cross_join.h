#include "../functional/config.h"
#include "../functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {
    namespace internal {

        /**
         *  CROSS JOIN holder.
         *  T is joined type which represents any mapped table.
         */
        template<class T>
        struct cross_join_t {
            using type = T;
        };

        template<class T>
        using is_cross_join = polyfill::is_specialization_of<T, cross_join_t>;
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  CROSS JOIN function. Usage:
     *  `cross_join<User>();`
     */
    template<class T>
    internal::cross_join_t<T> cross_join() {
        return {};
    }
}
