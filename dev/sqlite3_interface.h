#pragma once

/** @file Umbrella header for sqlite_orm's interface to the SQLite C library, the headers in `sqlite3/`.
 *
 *        Deliberately not named `sqlite3.h`: the amalgamation resolves `#include <sqlite3.h>` against `dev/`
 *        as well, and would inline this header in place of SQLite's.
 */

#include "sqlite3/sqlite3_config.h"
#include "sqlite3/sqlite3_types.h"
#include "sqlite3/sqlite3_errors.h"
#include "sqlite3/sqlite3_deleters.h"
#include "sqlite3/sqlite3_statements.h"
