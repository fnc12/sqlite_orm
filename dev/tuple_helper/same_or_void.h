#pragma once

namespace sqlite_orm {
    namespace internal {

        /**
         *  Accepts any number of arguments and evaluates `type` alias as T if all arguments are the same or void otherwise
         */
        template<class... Args>
        struct same_or_void {
            using type = void;
        };

        template<class A>
        struct same_or_void<A> {
            using type = A;
        };

        template<class A>
        struct same_or_void<A, A> {
            using type = A;
        };

        template<class... Args>
        using same_or_void_t = typename same_or_void<Args...>::type;

        template<class A, class... Args>
        struct same_or_void<A, A, Args...> : same_or_void<A, Args...> {};

        template<class Pack>
        struct same_or_void_of;

        template<template<class...> class Pack, class... Types>
        struct same_or_void_of<Pack<Types...>> : same_or_void<Types...> {};

        template<class Pack>
        using same_or_void_of_t = typename same_or_void_of<Pack>::type;
    }
}
