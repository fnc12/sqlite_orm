#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <utility>
#endif

namespace sqlite_orm {
    namespace internal {

        template<class T, class X>
        struct match_t {
            using mapped_type = T;
            using argument_type = X;

            argument_type argument;
        };
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T, class X>
    internal::match_t<T, X> match(X argument) {
        return {std::move(argument)};
    }
}
