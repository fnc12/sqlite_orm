#pragma once
#include "../../dev/functional/start_macros.h"
// Although every header file typically needs to contain all the necessary files, the configuration is an exception (as it would be easily forgotten).
// Therefore, we include the configuration and all underlying C++ core features to make them universally available.
#include "../../dev/functional/config.h"

// the SQLite C library interface
#include "../../dev/sqlite3_interface.h"

// sqlite_orm's own public leaf headers
#include "../../dev/error_code.h"
#include "../../dev/serialization/quoting.h"

// storage, and the manifests of the definitions
#include "../../dev/storage.h"
#include "../../dev/node_definitions.h"
#include "../../dev/node_algorithm_definitions.h"
#include "../../dev/interface_definitions.h"
#include "../../dev/get_prepared_statement.h"

// what SQLite provides: built-in collations, VFSes, functions and database objects
#include "../../dev/builtin/collations.h"
#include "../../dev/builtin/vfs.h"
#include "../../dev/builtin/functions.h"
#include "../../dev/builtin/dbos.h"

// the carray extension
#include "../../dev/carray.h"

#include "../../dev/functional/finish_macros.h"
