#pragma once

/*
 *  The public configuration: detection of the language, standard library, compiler and platform,
 *  the SQLite version and feature macros, and sqlite_orm's capabilities.
 *  Macros only, no declarations.
 */

#include "capabilities.h"
#include "toolchain/check_prerequisites.h"
#include "toolchain/compiler_quirks.h"
#include "toolchain/language_features.h"
#include "toolchain/platform.h"
#include "toolchain/stdlib_features.h"
