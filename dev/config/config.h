#pragma once

/*
 *  The public configuration: detection of the language, standard library, compiler and platform,
 *  the SQLite version and feature macros, and sqlite_orm's capabilities.
 *  Macros only, no declarations.
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

#include "toolchain/check_prerequisites.h"
#include "toolchain/language_features.h"
#include "toolchain/compiler_quirks.h"
#include "toolchain/platform.h"
#include "toolchain/stdlib_features.h"
#include "capabilities.h"

#endif
