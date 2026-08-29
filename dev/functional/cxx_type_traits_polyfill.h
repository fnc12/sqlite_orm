#pragma once

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#include <type_traits>
#endif

namespace sqlite_orm::internal::polyfill {
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
    struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
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
    inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;
#endif

#if 0  // proposed but not pursued
            using std::is_specialization_of, std::is_specialization_of_t, std::is_specialization_of_v;
#else
    // is_specialization_of: https://github.com/cplusplus/papers/issues/812

    template<typename Type, template<typename...> class Primary>
    inline constexpr bool is_specialization_of_v = false;

    template<template<typename...> class Primary, class... Types>
    inline constexpr bool is_specialization_of_v<Primary<Types...>, Primary> = true;

    template<typename Type, template<typename...> class Primary>
    struct is_specialization_of : std::bool_constant<is_specialization_of_v<Type, Primary>> {};
#endif

    template<typename...>
    inline constexpr bool always_false_v = false;

    template<size_t I>
    using index_constant = std::integral_constant<size_t, I>;
}

namespace sqlite_orm {
    namespace polyfill = internal::polyfill;
}
