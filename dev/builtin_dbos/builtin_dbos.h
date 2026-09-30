#pragma once

/** @file Umbrella header for the database objects SQLite provides itself:
 *        its schema table (`sqlite_master`/`sqlite_schema`), the eponymous virtual tables (`dbstat`,
 *        `generate_series`) and the virtual table modules (FTS5, R*Tree).
 */

#include "sqlite_schema.h"
#include "dbstat.h"
#include "generate_series.h"
#include "fts5.h"
#include "fts5_functions.h"
#include "fts5_deprecations.h"
#include "rtree.h"
