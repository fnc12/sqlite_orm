#pragma once

/** @file
 *  Purpose: tell which optional features sqlite_orm offers with the C++ language, standard library, platform and
 *  SQLite in use.
 *
 *  Intention: the capabilities tier of the configuration gathered by `config.h`, built on the other tiers;
 *  sqlite_orm's headers and its consumers test `SQLITE_ORM_WITH_*` for a feature.
 */

#include "../sqlite3/sqlite3_config.h"
#include "toolchain/language_features.h"
#include "toolchain/platform.h"
#include "toolchain/stdlib_features.h"

#if (defined(SQLITE_ORM_CPP_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_CPP_CONSTEVAL_SUPPORTED)) &&      \
    (defined(SQLITE_ORM_CPP20_LIB_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif

#define SQLITE_ORM_WITH_CTE

// the deprecated spelling of `SQLITE_ORM_OMIT_CODECVT`, which omits the `std::wstring` support
#if defined(SQLITE_ORM_OMITS_CODECVT) && !defined(SQLITE_ORM_OMIT_CODECVT)
#define SQLITE_ORM_OMIT_CODECVT
#endif

#if defined(SQLITE_ORM_CPP_REFLECTION_SUPPORTED)
#define SQLITE_ORM_WITH_VIEW
#endif

// The asynchronous storage: its I/O engine is io_uring (Linux), its fibers switch stacks with inline assembly
// written for GCC and Clang on x86_64 and aarch64.
#if (defined(SQLITE_ORM_PLATFORM_LINUX) && !defined(SQLITE_ORM_PLATFORM_ANDROID)) &&                                   \
    (defined(SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED) &&                                                             \
     !defined(SQLITE_ORM_INTERNAL_BROKEN_COROUTINE_TEMPORARIES)) &&                                                    \
    (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__aarch64__))
#define SQLITE_ORM_WITH_ASYNC
#endif
