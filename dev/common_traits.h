#pragma once

#include <type_traits>  //  std::true_type, std::false_type, std::declval
#include <memory>  // std::shared_ptr, std::unique_ptr, make functions..

namespace sqlite_orm {

    namespace internal {

        /*
         * This is because of bug in MSVC, for more information, please visit
         * https://stackoverflow.com/questions/34672441/stdis-base-of-for-template-classes/34672753#34672753
         */
#if defined(_MSC_VER)
        template<template<typename...> class Base, typename Derived>
        struct is_base_of_template_impl {
            template<typename... Ts>
            static constexpr std::true_type test(const Base<Ts...> *);

            static constexpr std::false_type test(...);

            using type = decltype(test(std::declval<Derived *>()));
        };

        template<typename Derived, template<typename...> class Base>
        using is_base_of_template = typename is_base_of_template_impl<Base, Derived>::type;

#else

        template<template<typename...> class C, typename... Ts>
        std::true_type is_base_of_template_impl(const C<Ts...> *);

        template<template<typename...> class C>
        std::false_type is_base_of_template_impl(...);

        template<typename T, template<typename...> class C>
        using is_base_of_template = decltype(is_base_of_template_impl<C>(std::declval<T *>()));
#endif
    }

    /**
     *  Specialization for optional type (std::shared_ptr / std::unique_ptr).
     */
    template<typename T>
    struct is_std_ptr : std::false_type {};

    template<typename T>
    struct is_std_ptr<std::shared_ptr<T>> : std::true_type {
        using element_type = T;

        static std::shared_ptr<T> make(const T &v) {
            return std::make_shared<T>(v);
        }
    };

    template<typename T>
    struct is_std_ptr<std::unique_ptr<T>> : std::true_type {
        using element_type = T;

        static std::unique_ptr<T> make(const T &v) {
            return std::make_unique<T>(v);
        }
    };

    namespace internal {
        template<template<class...> class TT, class U>
        struct is_template_matches_type : std::false_type {};

        template<template<class...> class TT, class... Args>
        struct is_template_matches_type<TT, TT<Args...>> : std::true_type {};
    }
}
