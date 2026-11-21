#pragma once

/** @file
 *  Purpose: identify the compiler that translates sqlite_orm.
 *
 *  Intention: the compiler tier of the configuration gathered by `config.h`; `compiler_quirks.h` keys its workarounds
 *  on it. Its macros are named `SQLITE_ORM_COMPILER_<maker>_<compiler>`, the compiler being the one it presents itself
 *  as: `SQLITE_ORM_COMPILER_GNU_GCC` is GNU's own GCC, as Clang defines `__GNUC__` too, and
 *  `SQLITE_ORM_COMPILER_CLANG_MSVC` is Clang presenting itself as MSVC.
 */

#if defined(_MSC_VER) && !defined(__clang__)
#define SQLITE_ORM_COMPILER_MS_MSVC
#endif

#if defined(__clang__) && defined(_MSC_VER)
#define SQLITE_ORM_COMPILER_CLANG_MSVC
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define SQLITE_ORM_COMPILER_GNU_GCC
#endif
