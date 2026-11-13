#pragma once

/** @file
 *  Purpose: gather the database objects SQLite provides.
 *
 *  Intention: the umbrella of `dbos/`, placed by `docs/internals/source-layout.md`; the single header takes it, through
 *  `third_party/amalgamate/sqlite_orm.json`, in place of the files behind it.
 */

#include "dbos/builtin_tables.h"
#include "dbos/dbstat.h"
#include "dbos/fts5.h"
#include "dbos/fts5_deprecations.h"
#include "dbos/generate_series.h"
#include "dbos/pragma_tables.h"
#include "dbos/rtree.h"
#include "dbos/sqlite_schema.h"
