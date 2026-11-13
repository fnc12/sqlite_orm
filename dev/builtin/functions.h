#pragma once

/** @file
 *  Purpose: gather the built-in SQL functions.
 *
 *  Intention: the umbrella of `functions/`, one header per family as SQLite documents them, placed by
 *  `docs/internals/source-layout.md`; the single header takes it, through `third_party/amalgamate/sqlite_orm.json`, in
 *  place of the files behind it.
 */

#include "functions/aggregate.h"
#include "functions/core.h"
#include "functions/datetime.h"
#include "functions/fts5_auxiliary.h"
#include "functions/json.h"
#include "functions/math.h"
#include "functions/window.h"
