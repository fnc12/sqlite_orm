#pragma once

/*
 *  This header defines macros for circumventing compiler quirks on which sqlite_orm depends.
 *  May amend cxx_core_features.h
 */

#ifdef __clang__
#define SQLITE_ORM_DO_PRAGMA(...) _Pragma(#__VA_ARGS__)
#endif

#ifdef __clang__
#define SQLITE_ORM_CLANG_SUPPRESS(warnoption, ...)                                                                     \
    SQLITE_ORM_DO_PRAGMA(clang diagnostic push)                                                                        \
    SQLITE_ORM_DO_PRAGMA(clang diagnostic ignored warnoption)                                                          \
    __VA_ARGS__                                                                                                        \
    SQLITE_ORM_DO_PRAGMA(clang diagnostic pop)
#else
#define SQLITE_ORM_CLANG_SUPPRESS(warnoption, ...) __VA_ARGS__
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define SQLITE_ORM_MS_MSVC
#endif

#if defined(__clang__) && defined(_MSC_VER)
#define SQLITE_ORM_CLANG_MSVC
#endif

#ifdef SQLITE_ORM_MS_MSVC
#define SQLITE_ORM_DO_PRAGMA(...) __pragma(__VA_ARGS__)
#endif

#ifdef SQLITE_ORM_MS_MSVC
#define SQLITE_ORM_MSVC_SUPPRESS(warncode, ...) SQLITE_ORM_DO_PRAGMA(warning(suppress : warncode))
#else
#define SQLITE_ORM_MSVC_SUPPRESS(warncode, ...) __VA_ARGS__
#endif

// msvc has the bad habit of diagnosing overalignment of types with an explicit alignment specifier.
#define SQLITE_ORM_MSVC_SUPPRESS_OVERALIGNMENT(...) SQLITE_ORM_MSVC_SUPPRESS(4324, __VA_ARGS__)

#if defined(SQLITE_ORM_MS_MSVC) && (_MSC_VER < 1920)
#define SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
// Type replacement may fail if an alias template has a non-type template parameter from a dependent expression in it,
// `e.g. template<class T> using is_something = std::bool_constant<is_something_v<T>>;`
// Remedy, e.g.: use a derived struct: `template<class T> struct is_somthing : std::bool_constant<is_something_v<T>>;`
#define SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR
#endif

// These compilers are known to have problems with alias templates in SFINAE contexts:
// clang 3.5
// gcc 8.3
// msvc 15.9
// Type replacement may fail if an alias template has dependent expression or decltype in it.
// In these cases we have to use helper structures to break down the type alias.
// Note that the detection of specific compilers is so complicated because some compilers emulate other compilers,
// so we simply exclude all compilers that do not support C++20, even though this test is actually inaccurate.
#if (defined(SQLITE_ORM_MS_MSVC) && (_MSC_VER < 1920)) || (!defined(SQLITE_ORM_MS_MSVC) && (__cplusplus < 202002L))
#define SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE
#endif

// overwrite SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#if (__cpp_nontype_template_args < 201911L) &&                                                                         \
    (defined(__clang__) && (__clang_major__ >= 12) && (__cplusplus >= 202002L))
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

// clang 10 chokes on concepts that don't depend on template parameters;
// when it tries to instantiate an expression in a requires expression, which results in an error,
// the compiler reports an error instead of dismissing the templated function.
#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && (defined(__clang__) && (__clang_major__ == 10))
#define SQLITE_ORM_BROKEN_NONTEMPLATE_CONCEPTS
#endif

#if defined(__clang__) && (__clang_major__ <= 15)
#define SQLITE_ORM_BROKEN_CPP20_VIEWS
#endif
