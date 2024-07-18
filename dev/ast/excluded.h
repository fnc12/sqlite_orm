#pragma once

#ifndef _IMPORT_STD_MODULE
#include <utility>  //  std::move
#endif

namespace sqlite_orm {
    namespace internal {

        template<class T>
        struct excluded_t {
            using expression_type = T;

            expression_type expression;
        };
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    template<class T>
    internal::excluded_t<T> excluded(T expression) {
        return {std::move(expression)};
    }
}
