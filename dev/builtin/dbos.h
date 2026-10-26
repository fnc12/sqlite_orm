#pragma once

/** @file Umbrella header for the database objects SQLite provides itself:
 *        its schema table (`sqlite_master`/`sqlite_schema`), the eponymous virtual tables (`dbstat`,
 *        `generate_series`, `pragma_table_info`/`pragma_table_xinfo`), the bundle `make_builtin_tables()`
 *        and the virtual table modules (FTS5, R*Tree).
 */

#include "dbos/builtin_tables.h"
#include "dbos/dbstat.h"
#include "dbos/fts5.h"
#include "dbos/fts5_deprecations.h"
#include "dbos/fts5_functions.h"
#include "dbos/generate_series.h"
#include "dbos/pragma_table_info.h"
#include "dbos/rtree.h"
#include "dbos/sqlite_schema.h"
