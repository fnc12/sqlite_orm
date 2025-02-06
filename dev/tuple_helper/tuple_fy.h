#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>
#endif

namespace sqlite_orm {

    namespace internal {

        template<typename T>
        struct tuplify {
            using type = std::tuple<T>;
        };
        template<typename... Ts>
        struct tuplify<std::tuple<Ts...>> {
            using type = std::tuple<Ts...>;
        };

        template<typename T>
        using tuplify_t = typename tuplify<T>::type;
    }
}
