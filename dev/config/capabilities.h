#pragma once

/*
 *  The optional features sqlite_orm offers with the language, standard library and SQLite in use.
 */

#include "toolchain/language_features.h"
#include "toolchain/stdlib_features.h"
#include "../sqlite3/sqlite3_config.h"

#if (defined(SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_CONSTEVAL_SUPPORTED)) &&              \
    (defined(SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif

#define SQLITE_ORM_WITH_CTE

#if defined(SQLITE_ORM_REFLECTION_SUPPORTED)
#define SQLITE_ORM_WITH_VIEW
#endif
