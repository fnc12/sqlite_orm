#pragma once

#include <type_traits>  //  std::true_type, std::false_type, std::declval

namespace sqlite_orm {

    namespace internal {

        /*
         * This is because of bug in MSVC, for more information, please visit
         * https://stackoverflow.com/questions/34672441/stdis-base-of-for-template-classes/34672753#34672753
         */
#ifdef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<template<typename...> class Base>
        struct is_base_of_template_impl {
            template<typename... Ts>
            static constexpr std::true_type test(const Base<Ts...>&);

            static constexpr std::false_type test(...);
        };

        template<typename T, template<typename...> class C>
        using is_base_of_template = decltype(is_base_of_template_impl<C>::test(std::declval<T>()));
#else
        template<template<typename...> class C, typename... Ts>
        std::true_type is_base_of_template_impl(const C<Ts...>&);

        template<template<typename...> class C>
        std::false_type is_base_of_template_impl(...);

        template<typename T, template<typename...> class C>
        using is_base_of_template = decltype(is_base_of_template_impl<C>(std::declval<T>()));
#endif

        template<typename T, template<typename...> class C>
        SQLITE_ORM_INLINE_VAR constexpr bool is_base_of_template_v = is_base_of_template<T, C>::value;
    }
}
