#pragma once

#include <tuple>  //  std::tuple
#include <type_traits>  //  std::enable_if

namespace sqlite_orm {
    namespace internal {

        template<class T, template<class> class C, class SFINAE = void>
        struct find_in_tuple;

        template<template<class> class C>
        struct find_in_tuple<std::tuple<>, C, void> {
            using type = void;
        };

        template<class H, class... Args, template<class> class C>
        struct find_in_tuple<std::tuple<H, Args...>, C, typename std::enable_if<C<H>::value>::type> {
            using type = H;
        };

        template<class H, class... Args, template<class> class C>
        struct find_in_tuple<std::tuple<H, Args...>, C, typename std::enable_if<!C<H>::value>::type> {
            using type = typename find_in_tuple<std::tuple<Args...>, C>::type;
        };
    }
}
