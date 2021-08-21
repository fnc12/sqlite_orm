#pragma once

#include <tuple>

namespace sqlite_orm {
    namespace internal {
        template<typename T>
        struct just_type {
            using type = T;
        };

        template<typename... T>
        using tuple_t = std::tuple<just_type<T>...>;

        template<typename... T>
        struct valuebased_tuple;

        template<typename... Ts>
        struct valuebased_tuple<std::tuple<Ts...>> {
            using type = tuple_t<Ts...>;
        };
    }
}
