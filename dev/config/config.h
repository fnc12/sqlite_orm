#pragma once

/** @file
 *  Purpose: gather the public configuration, macros only.
 *
 *  Intention: gathers one header per tier:
 *  - `toolchain/language_features.h`, the C++ language features;
 *  - `toolchain/stdlib_features.h`, the C++ standard library features;
 *  - `toolchain/compiler.h` and `toolchain/platform.h`, the compiler and the platform;
 *  - `sqlite3/sqlite3_config.h`, the SQLite features;
 *  - `capabilities.h`, sqlite_orm's capabilities;
 *  - `toolchain/compiler_quirks.h`, the internal tier: published only because the tiers above rest on it, and subject
 *    to change without notice.
 *  `internal_macros.h` builds sqlite_orm's own spelling on the tiers, and `toolchain/check_prerequisites.h`, outside
 *  them, rejects a toolchain short of C++17.
 *
 *  Published as <sqlite_orm/sqlite_orm_config.h> for consumers of the named module, which cannot export macros:
 *  include it ahead of `import sqlite_orm;` and `import std;`.
 *  The standard permits either order, but GCC rejects including it after either import: it does not merge
 *  the standard library's inline definitions from `<version>` with those reachable from the module
 *  [see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99000].
 *  The include guard is shared by both published headers, which each contain this header.
 */

#ifndef SQLITE_ORM_CONFIG_INCLUDED
#define SQLITE_ORM_CONFIG_INCLUDED

#include "capabilities.h"
#include "toolchain/compiler.h"
#include "toolchain/compiler_quirks.h"
#include "toolchain/language_features.h"
#include "toolchain/platform.h"
#include "toolchain/stdlib_features.h"

#endif
