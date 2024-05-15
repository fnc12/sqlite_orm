#pragma once

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif

#ifdef SQLITE_ORM_CONFIG_DISABLE_STATIC_ASSERTIONS
#define SQLITE_ORM_STASSERT(...)
#else
#define SQLITE_ORM_STASSERT(...) static_assert(__VA_ARGS__)
#endif
