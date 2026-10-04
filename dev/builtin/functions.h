#pragma once

/** @file Umbrella header for the built-in SQL functions, one header per family as SQLite documents them.
 *
 *        In C++20 builds a built-in function is a definition object of `ast/builtin_function.h` (name + overload set),
 *        published in the `sqlite_orm` namespace as a copy - or wrapped by a function template where the public
 *        function takes the return type as a template argument, shares its name with other function templates or
 *        checks its arguments. The C++17 branch keeps the legacy factories over `builtin_function_t`.
 *
 *        A built-in whose name is also that of a C library function in the global namespace - `abs`, `round`,
 *        `time`, `random`, `strftime`, `printf` - is wrapped as well: under `using namespace sqlite_orm;` an object
 *        and a function of the same name are an ambiguous lookup, whereas two functions are an overload set.
 */

#include "functions/core.h"
#include "functions/datetime.h"
#include "functions/aggregate.h"
#include "functions/math.h"
#include "functions/json.h"
#include "functions/window.h"
