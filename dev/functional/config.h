#pragma once

#include "cxx_universal.h"

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

#if(defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_INLINE_VARIABLES_SUPPORTED) &&         \
    defined(SQLITE_ORM_CONSTEVAL_SUPPORTED)) &&                                                                        \
    (defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif
