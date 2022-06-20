#pragma once

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

// clang has the bad habit of diagnosing missing brace-init-lists when constructing aggregates with base classes.
// This is a false positive, since the C++ standard is quite clear that braces for nested or base objects may be omitted,
// see https://en.cppreference.com/w/cpp/language/aggregate_initialization:
// "The braces around the nested initializer lists may be elided (omitted),
//  in which case as many initializer clauses as necessary are used to initialize every member or element of the corresponding subaggregate,
//  and the subsequent initializer clauses are used to initialize the following members of the object."
// In this sense clang should only warn about missing field initializers.
// Because we know what we are doing, we suppress the diagnostic message
#define SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(...) SQLITE_ORM_CLANG_SUPPRESS("-Wmissing-braces", __VA_ARGS__)

#if defined(_MSC_VER) && !defined(__clang__)  // MSVC
#define SQLITE_ORM_MSVC_EMPTYBASES __declspec(empty_bases)
#else
#define SQLITE_ORM_MSVC_EMPTYBASES
#endif

#if defined(_MSC_VER) && !defined(__clang__)  // MSVC

#if __cplusplus < 202002L
#define SQLITE_ORM_WORKAROUND_MSVC_MULTIPLECTOR_106654
#endif

#if _MSC_VER < 1920
#define SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
#define SQLITE_ORM_BROKEN_CONSTEXPR_DELEGATING_CTORS
#endif

#elif defined(__clang__) && defined(_MSC_VER)  // Clang-cl (Clang for Windows)

#elif defined(__clang__) && defined(__apple_build_version__)  // Apple's Clang

#elif defined(__clang__)  // genuine Clang

#elif defined(__GNUC__)  // GCC

#if __GNUC__ < 11 || (__GNUC__ == 11 && __GNUC_MINOR__ < 3)
#define SQLITE_ORM_BROKEN_GCC_ALIAS_TARGS_84785
#endif

#endif
