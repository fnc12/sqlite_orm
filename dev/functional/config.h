#pragma once

#include "cxx_universal.h"

#ifdef BUILD_SQLITE_ORM_MODULE
#define SQLITE_ORM_EXPORT export
#else
#define SQLITE_ORM_EXPORT
#endif

#if SQLITE_ORM_HAS_INCLUDE(<version>)
#include <version>
#endif

#ifdef SQLITE_ORM_CONSTEXPR_LAMBDAS_SUPPORTED
#define SQLITE_ORM_CONSTEXPR_LAMBDA_CPP17 constexpr
#else
#define SQLITE_ORM_CONSTEXPR_LAMBDA_CPP17
#endif

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#define SQLITE_ORM_INLINE_VAR inline
#else
#define SQLITE_ORM_INLINE_VAR
#endif

#ifdef SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
#define SQLITE_ORM_CONSTEXPR_IF constexpr
#else
#define SQLITE_ORM_CONSTEXPR_IF
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

#ifdef SQLITE_ORM_CONSTEVAL_SUPPORTED
#define SQLITE_ORM_CONSTEVAL consteval
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#endif

#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && __cpp_lib_concepts >= 202002L
#define SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#endif

#if __cpp_lib_ranges >= 201911L
#define SQLITE_ORM_CPP20_RANGES_SUPPORTED
#endif

#if __cpp_lib_semaphore >= 201907L
#define SQLITE_ORM_CPP20_SEMAPHORE_SUPPORTED
#endif

// C++20 or later (unfortunately there's no feature test macro).
// Stupidly, clang says C++20, but `std::default_sentinel_t` was only implemented in libc++ 13 and libstd++-v3 10
// (the latter is used on Linux).
// gcc got it right and reports C++20 only starting with v10.
// The check here doesn't care and checks the library versions in use.
//
// Another way of detection might be the feature-test macro __cpp_lib_concepts
#if (__cplusplus >= 202002L) &&                                                                                        \
    ((!_LIBCPP_VERSION || _LIBCPP_VERSION >= 13000) && (!_GLIBCXX_RELEASE || _GLIBCXX_RELEASE >= 10))
#define SQLITE_ORM_STL_HAS_DEFAULT_SENTINEL
#endif

#if (defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_INLINE_VARIABLES_SUPPORTED) &&        \
     defined(SQLITE_ORM_CONSTEVAL_SUPPORTED)) &&                                                                       \
    (defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif

#if defined(SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED) && defined(SQLITE_ORM_IF_CONSTEXPR_SUPPORTED)
#define SQLITE_ORM_WITH_CTE
#endif

// define the inline namespace "literals" so that it is available even if it was not introduced by a feature
namespace sqlite_orm {
    inline namespace literals {}
}
