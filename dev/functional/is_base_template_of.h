#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::true_type, std::false_type, std::declval
#endif

namespace sqlite_orm {

    namespace internal {

        /*
         * This is because of bug in MSVC, for more information, please visit
         * https://stackoverflow.com/questions/34672441/stdis-base-of-for-template-classes/34672753#34672753
         */
#ifdef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<template<typename...> class Base>
        struct is_base_template_of_impl {
            template<typename... Ts>
            static constexpr std::true_type test(const Base<Ts...>&);

            static constexpr std::false_type test(...);
        };

        template<template<typename...> class Base, typename T>
        using is_base_template_of = decltype(is_base_template_of_impl<Base>::test(std::declval<T>()));
#else
        template<template<typename...> class Base, typename... Ts>
        std::true_type is_base_template_of_impl(const Base<Ts...>&);

        template<template<typename...> class Base>
        std::false_type is_base_template_of_impl(...);

        template<template<typename...> class Base, typename T>
        using is_base_template_of = decltype(is_base_template_of_impl<Base>(std::declval<T>()));
#endif

        template<template<typename...> class Base, typename T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_base_template_of_v = is_base_template_of<Base, T>::value;
    }
}
