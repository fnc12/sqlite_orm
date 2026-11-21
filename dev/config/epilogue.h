#pragma once

/** @file
 *  Purpose: restore the compiler warnings and the `min`/`max` macros `prologue.h` suspended.
 *
 *  Intention: paired with `prologue.h`; both variants of <sqlite_orm/sqlite_orm.h> place it last.
 */

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
__pragma(pop_macro("max"))
__pragma(pop_macro("min"))
#endif
