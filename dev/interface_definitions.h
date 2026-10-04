#pragma once

/** @file Manifest of the out-of-class member definitions in `implementations/`.
 *
 *  Each of those files defines members of a schema, storage or AST node class whose bodies need headers the
 *  declaring header should not depend on itself - implementation machinery private to how the member does its work.
 *  They are included here once, after all declarations.
 */

#include "implementations/column_definitions.h"
#include "implementations/table_definitions.h"
#include "implementations/storage_definitions.h"
#include "implementations/dynamic_set_definitions.h"
