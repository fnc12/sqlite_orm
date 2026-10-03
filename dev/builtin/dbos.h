#pragma once

/** @file Umbrella header for the database objects SQLite provides itself:
 *        its schema table (`sqlite_master`/`sqlite_schema`), the eponymous virtual tables (`dbstat`,
 *        `generate_series`) and the virtual table modules (FTS5, R*Tree).
 */

#include "dbos/sqlite_schema.h"
#include "dbos/dbstat.h"
#include "dbos/generate_series.h"
#include "dbos/fts5.h"
#include "dbos/fts5_functions.h"
#include "dbos/fts5_deprecations.h"
#include "dbos/rtree.h"
