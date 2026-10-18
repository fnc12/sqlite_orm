#pragma once

/*
 *  The public configuration: detection of the language, standard library, compiler and platform,
 *  the SQLite version and feature macros, and sqlite_orm's capabilities.
 *  Macros only, no declarations.
 *
 *  Published as <sqlite_orm/sqlite_orm_config.h> for consumers of the named module, which cannot export macros:
 *  include it ahead of `import sqlite_orm;`.
 *  The include guard is shared by both published headers, which each contain this header.
 */

#ifndef SQLITE_ORM_CONFIG_INCLUDED
#define SQLITE_ORM_CONFIG_INCLUDED

#include "prerequisites.h"
#include "language_features.h"
#include "compiler_quirks.h"
#include "platform.h"
#include "stdlib_features.h"
#include "capabilities.h"

#endif
