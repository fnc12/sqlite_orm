#pragma once

#include <tuple>

//  got from here https://2019.cppconf-moscow.ru/talks/2zq0btoxldq6tmo3g7cvuo/
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