#pragma once

#include "cxx_universal.h"

#ifdef SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#define SQLITE_ORM_INLINE_VAR inline
#else
#define SQLITE_ORM_INLINE_VAR
#endif

#if SQLITE_ORM_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif

#ifdef SQLITE_ORM_CONSTEVAL_SUPPORTED
#define SQLITE_ORM_CONSTEVAL consteval
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#endif

#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) &&                 \
    defined(SQLITE_ORM_INLINE_VARIABLES_SUPPORTED)
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif
