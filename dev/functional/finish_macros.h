#pragma once

#ifdef SQLITE_ORM_CONFIG_DISABLE_STATIC_ASSERTIONS
#undef SQLITE_ORM_STASSERT
#endif

#if defined(_MSC_VER)
__pragma(pop_macro("max"))
__pragma(pop_macro("min"))
#endif
