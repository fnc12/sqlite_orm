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

#if (defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_CONSTEVAL_SUPPORTED)) &&              \
    (defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif

#define SQLITE_ORM_WITH_CTE

#if defined(SQLITE_ORM_REFLECTION_SUPPORTED)
#define SQLITE_ORM_WITH_VIEW
#endif

// The asynchronous storage: its I/O engine is io_uring (Linux), its fibers switch stacks with inline assembly
// written for GCC and Clang on x86_64 and aarch64.
#if (defined(SQLITE_ORM_LINUX) && !defined(SQLITE_ORM_ANDROID)) &&                                                     \
    (defined(SQLITE_ORM_CPP20_COROUTINES_SUPPORTED) && !defined(SQLITE_ORM_BROKEN_COROUTINE_TEMPORARIES)) &&           \
    (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__aarch64__))
#define SQLITE_ORM_WITH_ASYNC
#endif
