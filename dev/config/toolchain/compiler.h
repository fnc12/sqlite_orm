#pragma once

/** @file
 *  Purpose: identify the compiler that translates sqlite_orm.
 *
 *  Intention: the compiler tier of the configuration gathered by `config.h`; `compiler_quirks.h` keys its workarounds
 *  on it. Each name gives the compiler's maker, then the compiler it presents itself as: `SQLITE_ORM_GNU_GCC` is GNU's
 *  own GCC, as Clang defines `__GNUC__` too, and `SQLITE_ORM_CLANG_MSVC` is Clang presenting itself as MSVC.
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
