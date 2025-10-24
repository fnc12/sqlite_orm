#pragma once

#include "cxx_core_features.h"

#ifdef SQLITE_ORM_IMPORT_STD_MODULE
#include <version>
#elif __has_include(<string_view>)
#include <string_view>
#endif

#if __cpp_lib_string_view >= 201606L
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#endif
