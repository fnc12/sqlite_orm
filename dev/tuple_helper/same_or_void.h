#pragma once
#include <type_traits>  //  std::common_type

namespace sqlite_orm {
    namespace internal {

        /**
         *  Accepts any number of arguments and evaluates a nested `type` typename as `T` if all arguments are the same, otherwise `void`.
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
        struct common_type_of;

        template<template<class...> class Pack, class... Types>
        struct common_type_of<Pack<Types...>> : std::common_type<Types...> {};

        /**
         *  Accepts a pack of types and defines a nested `type` typename to a common type if possible, otherwise nonexistent.
         *  
         *  @note: SFINAE friendly like `std::common_type`.
         */
        template<class Pack>
        using common_type_of_t = typename common_type_of<Pack>::type;
    }
}
