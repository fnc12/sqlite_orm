#pragma once

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif  // defined(_MSC_VER)

#include <ciso646>  //  due to #166

#if __cplusplus >= 201703L  // use of C++17 or higher
// Enables use of std::optional in SQLITE_ORM.
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#define SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
#define SQLITE_ORM_INLINE_VAR inline
#if __cplusplus >= 202002L  // use of C++20 or higher
#define SQLITE_ORM_CONSTEVAL consteval
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif
#else
#define SQLITE_ORM_INLINE_VAR
#define SQLITE_ORM_CONSTEVAL constexpr
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif
