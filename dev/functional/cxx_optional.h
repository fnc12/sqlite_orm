#pragma once

#include "cxx_core_features.h"

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#elif __has_include(<optional>)
#include <optional>
#endif

#if __cpp_lib_optional >= 201606L
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#endif
