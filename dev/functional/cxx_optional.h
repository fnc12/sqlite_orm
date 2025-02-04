#pragma once

#include "cxx_core_features.h"

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#if SQLITE_ORM_HAS_INCLUDE(<optional>)
#include <optional>
#endif
#endif

#if __cpp_lib_optional >= 201606L
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#endif
