#pragma once

/** @file Manifest of the out-of-class member definitions, each in a `_definitions.h` header next to the class it completes.
 *
 *  Each of those files defines members of a schema, storage or AST node class whose bodies need headers the
 *  declaring header should not depend on itself - implementation machinery private to how the member does its work.
 *  They are included here once, after all declarations.
 */

#include "schema/column_definitions.h"
#include "schema/table_definitions.h"
#include "execution/storage_definitions.h"
#include "ast/crud/set_definitions.h"
