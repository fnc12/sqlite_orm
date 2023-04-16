#pragma once
#include <type_traits>

#include "cxx_universal.h"

namespace sqlite_orm {
    namespace internal {
        namespace polyfill {
#if __cpp_lib_void_t >= 201411L
            using std::void_t;
#else
            template<class...>
            using void_t = void;
#endif

#if __cpp_lib_bool_constant >= 201505L
            using std::bool_constant;
#else
            template<bool v>
            using bool_constant = std::integral_constant<bool, v>;
#endif

#if __cpp_lib_logical_traits >= 201510L && __cpp_lib_type_trait_variable_templates >= 201510L
            using std::conjunction;
            using std::conjunction_v;
            using std::disjunction;
            using std::disjunction_v;
            using std::negation;
            using std::negation_v;
#else
            template<typename...>
            struct conjunction : std::true_type {};
            template<typename B1>
            struct conjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct conjunction<B1, Bn...> : std::conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};
            template<typename... Bs>
            SQLITE_ORM_INLINE_VAR constexpr bool conjunction_v = conjunction<Bs...>::value;

            template<typename...>
            struct disjunction : std::false_type {};
            template<typename B1>
            struct disjunction<B1> : B1 {};
            template<typename B1, typename... Bn>
            struct disjunction<B1, Bn...> : std::conditional_t<bool(B1::value), B1, disjunction<Bn...>> {};
            template<typename... Bs>
            SQLITE_ORM_INLINE_VAR constexpr bool disjunction_v = disjunction<Bs...>::value;

            template<typename B>
            struct negation : bool_constant<!bool(B::value)> {};
            template<typename B>
            SQLITE_ORM_INLINE_VAR constexpr bool negation_v = negation<B>::value;
#endif

#if __cpp_lib_remove_cvref >= 201711L
            using std::remove_cvref, std::remove_cvref_t;
#else
            template<class T>
            struct remove_cvref : std::remove_cv<std::remove_reference_t<T>> {};

            template<class T>
            using remove_cvref_t = typename remove_cvref<T>::type;
#endif

#if __cpp_lib_type_identity >= 201806L
            using std::type_identity, std::type_identity_t;
#else
            template<class T>
            struct type_identity {
                using type = T;
            };

            template<class T>
            using type_identity_t = typename type_identity<T>::type;
#endif

#if 0  // __cpp_lib_detect >= 0L  //  library fundamentals TS v2, [meta.detect]
            using std::nonesuch;
            using std::detector;
            using std::is_detected, std::is_detected_v;
            using std::detected, std::detected_t;
            using std::detected_or, std::detected_or_t;
#else
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
            using detected = detector<nonesuch, void, Op, Args...>;

            template<template<class...> class Op, class... Args>
            using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

            template<class Default, template<class...> class Op, class... Args>
            using detected_or = detector<Default, void, Op, Args...>;

            template<class Default, template<class...> class Op, class... Args>
            using detected_or_t = typename detected_or<Default, Op, Args...>::type;

            template<template<class...> class Op, class... Args>
            SQLITE_ORM_INLINE_VAR constexpr bool is_detected_v = is_detected<Op, Args...>::value;
#endif

#if 0  // proposed but not pursued
            using std::is_specialization_of, std::is_specialization_of_t, std::is_specialization_of_v;
#else
            // is_specialization_of: https://github.com/cplusplus/papers/issues/812

            template<typename Type, template<typename...> class Primary>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v = false;

            template<template<typename...> class Primary, class... Types>
            SQLITE_ORM_INLINE_VAR constexpr bool is_specialization_of_v<Primary<Types...>, Primary> = true;

            template<typename Type, template<typename...> class Primary>
            struct is_specialization_of : bool_constant<is_specialization_of_v<Type, Primary>> {};
#endif

            template<typename...>
            SQLITE_ORM_INLINE_VAR constexpr bool always_false_v = false;

            template<size_t I>
            using index_constant = std::integral_constant<size_t, I>;
        }
    }

    namespace polyfill = internal::polyfill;
}
