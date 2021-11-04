#pragma once

#if defined(_MSC_VER)
#if defined(min)
__pragma(push_macro("min"))
#undef min
#define __RESTORE_MIN__
#endif
#if defined(max)
    __pragma(push_macro("max"))
#undef max
#define __RESTORE_MAX__
#endif
#endif  // defined(_MSC_VER)

#include <ciso646>  //  due to #166

#if __cplusplus >= 201703L  // use of C++17 or higher
// Enables use of std::optional in SQLITE_ORM.
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#define SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
#endif
