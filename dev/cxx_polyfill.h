#pragma once

#include <type_traits>

#include "start_macros.h"

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
#if __cplusplus < 201703L  // before C++17
            template<typename...>
            using void_t = void;

            template<bool v>
            using bool_constant = std::integral_constant<bool, v>;

            template<typename...>
            struct conjunction : std::true_type {};
            template<typename B1>
            struct conjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct conjunction<B1, Bn...> : std::conditional_t<B1::value, conjunction<Bn...>, B1> {};
            template<typename... Bs>
            constexpr bool conjunction_v = conjunction<Bs...>::value;

            template<typename...>
            struct disjunction : std::false_type {};
            template<typename B1>
            struct disjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct disjunction<B1, Bn...> : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};
            template<typename... Bs>
            constexpr bool disjunction_v = disjunction<Bs...>::value;
#else
            using std::bool_constant;
            using std::conjunction;
            using std::conjunction_v;
            using std::disjunction;
            using std::disjunction_v;
            using std::void_t;
#endif

#if __cplusplus < 202002L
            template<class T>
            struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};

            template<class T>
            using remove_cvref_t = typename remove_cvref<T>::type;
#else
            using std::remove_cvref;
            using std::remove_cvref_t;
#endif

#if __cplusplus < 202312L  // before C++23
            template<typename Type, template<typename...> class Primary>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v = false;

            template<template<typename...> class Primary, class... Types>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v<Primary<Types...>, Primary> = true;

            template<typename Type, template<typename...> class Primary>
            struct is_specialization_of : bool_constant<is_specialization_of_v<Type, Primary>> {};

            template<typename... T>
            using is_specialization_of_t = typename is_specialization_of<T...>::type;
#else
            using std::is_specialization_of, std::is_specialization_of_t, std::is_specialization_of_v;
#endif

            template<typename...>
            SQLITE_ORM_INLINE_VAR constexpr bool always_false_v = false;

            template<size_t I>
            using index_constant = std::integral_constant<size_t, I>;

#if 1  // library fundamentals TS v2
            struct nonesuch {
                ~nonesuch() = delete;
                nonesuch(const nonesuch&) = delete;
                void operator=(const nonesuch&) = delete;
            };

            template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
            struct detector {
                using value_t = std::false_type;
                using type = Default;
            };

            template<class Default, template<class...> class Op, class... Args>
            struct detector<Default, polyfill::void_t<Op<Args...>>, Op, Args...> {
                using value_t = std::true_type;
                using type = Op<Args...>;
            };

            template<template<class...> class Op, class... Args>
            using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

            template<template<class...> class Op, class... Args>
            using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

            template<class Default, template<class...> class Op, class... Args>
            using detected_or = detector<Default, void, Op, Args...>;

            template<class Default, template<class...> class Op, class... Args>
            using detected_or_t = typename detected_or<Default, Op, Args...>::type;

            template<template<class...> class Op, class... Args>
            SQLITE_ORM_INLINE_VAR constexpr bool is_detected_v = is_detected<Op, Args...>::value;
#endif

            template<template<typename...> typename Base, typename... Fs>
            Base<Fs...>& as_template_base(Base<Fs...>& base) {
                return base;
            }
            template<template<typename...> typename Base, typename... Fs>
            const Base<Fs...>& as_template_base(const Base<Fs...>& base) {
                return base;
            }
            template<template<typename...> typename Base, typename... Fs>
            Base<Fs...>&& as_template_base(Base<Fs...>&& base) {
                return std::move(base);
            }
            template<template<typename...> typename Base, typename... Fs>
            const Base<Fs...>&& as_template_base(const Base<Fs...>&& base) {
                return std::move(base);
            }

            /** @short Deduce template base specialization from a derived type */
            template<template<typename...> class Base, class Derived>
            using as_template_base_t = decltype(as_template_base<Base>(std::declval<Derived>()));

            template<template<typename...> class Base, class Derived, typename SFINAE = void>
            struct is_template_base_of : std::false_type {};

            template<template<typename...> class Base, class Derived>
            struct is_template_base_of<Base, Derived, polyfill::void_t<as_template_base_t<Base, Derived>>>
                : std::true_type {};

            template<template<typename...> class Base, class Derived>
            using is_template_base_of_t = typename is_template_base_of<Base, Derived>::type;

            template<template<typename...> class Base, class Derived>
            SQLITE_ORM_INLINE_VAR constexpr bool is_template_base_of_v = is_template_base_of<Base, Derived>::value;
        }
    }

    namespace polyfill = internal::polyfill;
}
