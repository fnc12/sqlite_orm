#pragma once

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif  // defined(_MSC_VER)

#include <iso646.h>  //  alternative operator representations
#include <stddef.h>  //  sqlite_orm is using size_t, ptrdiff_t everywhere, pull it in early

#include "cxx_core_features.h"
#include "cxx_compiler_quirks.h"
