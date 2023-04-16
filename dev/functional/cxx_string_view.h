#pragma once

#include "cxx_core_features.h"

#if SQLITE_ORM_HAS_INCLUDE(<string_view>)
#include <string_view>
#endif

#if __cpp_lib_string_view >= 201606L
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#endif
