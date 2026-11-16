#pragma once

/*
 *  The macros sqlite_orm's own code is spelled with: module export, keywords and attributes
 *  depending on the language version, and the suppression of compiler diagnostics.
 */

#include "config.h"

#ifdef BUILD_SQLITE_ORM_MODULE
#define SQLITE_ORM_EXPORT export
#else
#define SQLITE_ORM_EXPORT
#endif

#if !defined(SQLITE_ORM_MS_MSVC) || (_MSC_VER >= 1920)
#define SQLITE_ORM_SWITCH_MAYBE_UNUSED [[maybe_unused]]
#else
#define SQLITE_ORM_SWITCH_MAYBE_UNUSED
#endif

#if __cpp_lib_constexpr_functional >= 201907L
#define SQLITE_ORM_CONSTEXPR_CPP20 constexpr
#else
#define SQLITE_ORM_CONSTEXPR_CPP20
#endif

#if SQLITE_ORM_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif

#if SQLITE_ORM_HAS_CPP_ATTRIBUTE(likely) >= 201803L
#define SQLITE_ORM_CPP_LIKELY [[likely]]
#define SQLITE_ORM_CPP_UNLIKELY [[unlikely]]
#else
#define SQLITE_ORM_CPP_LIKELY
#define SQLITE_ORM_CPP_UNLIKELY
#endif

// note: Visual Studio 2019 v16.11 supports `consteval` but not for functions returning void.
#if defined(SQLITE_ORM_CONSTEVAL_SUPPORTED) && (!defined(SQLITE_ORM_MS_MSVC) || (_MSC_VER >= 1930))
#define SQLITE_ORM_CONSTEVAL consteval
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#endif

#ifdef SQLITE_ORM_STATIC_CALL_OPERATOR_SUPPORTED
#define SQLITE_ORM_STATIC_CALLOP static
#define SQLITE_ORM_OR_CONST_CALLOP
#else
#define SQLITE_ORM_STATIC_CALLOP
#define SQLITE_ORM_OR_CONST_CALLOP const
#endif

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
