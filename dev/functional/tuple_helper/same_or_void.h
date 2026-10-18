#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  //  std::same_as
#endif
#endif

namespace sqlite_orm::internal {
    /**
     *  Accepts any number of arguments and evaluates a nested `type` typename as `T` if all arguments are the same, otherwise `void`.
     */
    template<class... Args>
    struct same_or_void {
        using type = void;
    };

    template<class... Args>
    using same_or_void_t = typename same_or_void<Args...>::type;

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class A, std::same_as<A>... Rest>
    struct same_or_void<A, Rest...> {
        using type = A;
    };
#else
    template<class A>
    struct same_or_void<A> {
        using type = A;
    };

    template<class A>
    struct same_or_void<A, A> {
        using type = A;
    };

    template<class A, class... Args>
    struct same_or_void<A, A, Args...> : same_or_void<A, Args...> {};
#endif
}
