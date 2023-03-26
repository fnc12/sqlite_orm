#pragma once

#include "cxx_core_features.h"

#if SQLITE_ORM_HAS_INCLUDE(<optional>)
#include <optional>
#endif

#if __cpp_lib_optional >= 201606L
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#endif
