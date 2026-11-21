#pragma once

/** @file
 *  Purpose: identify the compiler that translates sqlite_orm.
 *
 *  Intention: part of the public configuration gathered by `config.h`; `compiler_quirks.h` keys its workarounds on it.
 */

#if defined(_MSC_VER) && !defined(__clang__)
#define SQLITE_ORM_MS_MSVC
#endif

#if defined(__clang__) && defined(_MSC_VER)
#define SQLITE_ORM_CLANG_MSVC
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define SQLITE_ORM_GNU_GCC
#endif
