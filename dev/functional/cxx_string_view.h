#pragma once

#include "cxx_core_features.h"

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#else
#if SQLITE_ORM_HAS_INCLUDE(<string_view>)
#include <string_view>
#endif
#endif

#if __cpp_lib_string_view >= 201606L
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#endif
