#pragma once

/** @file The top-level header of the library, when used as separate headers rather than as the amalgamation.
 *
 *  The order of the headers below matters, and `third_party/amalgamate/config.json` lists them in the same order.
 *  Vocabulary traits are declared `extern const` and defined by the headers of what they classify, which arrive here
 *  rather than through the headers using them. That suffices for evaluations inside templates, which happen in user
 *  code; a trait evaluated for a concrete type while the headers are processed needs its definition included ahead.
 */

#include "../../dev/functional/start_macros.h"
// Although every header file typically needs to contain all the necessary files, the configuration is an exception (as it would be easily forgotten).
// Therefore, we include the configuration and all underlying C++ core features to make them universally available.
#include "../../dev/functional/config.h"

// the SQLite C library interface
#include "../../dev/sqlite3_interface.h"

// sqlite_orm's own public leaf headers
#include "../../dev/error_code.h"
#include "../../dev/serialization/quoting.h"
// the pointer-passing interface; ahead of the storage, as it defines `is_pointer_arg_v` and `is_pointer_binding_v`,
// which `statement_binder` and `row_extractor` evaluate for the types they are instantiated with right away
#include "../../dev/marshaling/pointer_passing.h"
#include "../../dev/marshaling/carray_pointer_passing.h"

// the manifests of the definitions, ahead of the storage, so that every trait is defined before its first use
#include "../../dev/node_definitions.h"
#include "../../dev/node_algorithm_definitions.h"
// the storage, and the definitions of its interface
#include "../../dev/execution/storage.h"
#include "../../dev/interface_definitions.h"
#include "../../dev/execution/get_prepared_statement.h"

// what SQLite provides: built-in collations, VFSes, functions and database objects
#include "../../dev/builtin/collations.h"
#include "../../dev/builtin/vfs.h"
#include "../../dev/builtin/functions.h"
#include "../../dev/builtin/dbos.h"

#include "../../dev/functional/finish_macros.h"
