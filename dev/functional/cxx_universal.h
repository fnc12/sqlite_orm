#pragma once

/*
 *  This header makes central C++ functionality on which sqlite_orm depends universally available:
 *  - alternative operator representations
 *  - ::size_t, ::ptrdiff_t, std::nullptr_t
 *  - C++ core language feature macros
 *  - macros for dealing with compiler quirks
 *  - macros for exporting symbols from the C++ named module
 */

#include <iso646.h>  //  alternative operator representations
#ifndef _IMPORT_STD_MODULE
#include <cstddef>  //  sqlite_orm is using ::size_t, ::ptrdiff_t, std::nullptr_t everywhere, pull it in early
// earlier libcxx versions didn't make std::nullptr_t available in the global namespace via stddef.h,
// though it should have according to C++ documentation (see https://en.cppreference.com/w/cpp/types/std::nullptr_t#Notes).
using std::nullptr_t;
// Further note on the use of nullptr_t:
// msvc 14.40 has problems finding `::nullptr_t` within sqlite_orm when consuming sqlite_orm as a named module.
// Hence, sqlite_orm is internally using `std::nullptr_t` instead.
#endif

#include "cxx_core_features.h"
#include "cxx_compiler_quirks.h"
