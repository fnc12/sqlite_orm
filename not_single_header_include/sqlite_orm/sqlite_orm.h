#pragma once
#include "../../dev/functional/start_macros.h"
// pull in the SQLite3 configuration early, such that version and feature macros are globally available in sqlite_orm
#include "../../dev/functional/sqlite3_config.h"
// though each header is required to include everything it needs
// we include the configuration and all underlying c++ core features in order to make it universally available
#include "../../dev/functional/config.h"
#include "../../dev/storage.h"
#include "../../dev/interface_definitions.h"
#include "../../dev/get_prepared_statement.h"
#include "../../dev/carray.h"
#include "../../dev/functional/finish_macros.h"
