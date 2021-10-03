#pragma once

#include <tuple>  //  std::tuple

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class> class C>
        struct count_tuple;

        template<template<class> class C>
        struct count_tuple<std::tuple<>, C> {
            static constexpr const int value = 0;
        };

        template<class H, class... Args, template<class> class C>
        struct count_tuple<std::tuple<H, Args...>, C> {
            static constexpr const int value = C<H>::value + count_tuple<std::tuple<Args...>, C>::value;
        };
    }
}
