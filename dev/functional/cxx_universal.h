#pragma once

/*
 *  This header makes central C++ functionality on which sqlite_orm depends universally available:
 *  - alternative operator representations
 *  - ::size_t, ::ptrdiff_t, ::nullptr_t
 *  - C++ core language feature macros
 *  - macros for dealing with compiler quirks
 */

#include <iso646.h>  //  alternative operator representations
#include <cstddef>  //  sqlite_orm is using size_t, ptrdiff_t, nullptr_t everywhere, pull it in early

// earlier clang versions didn't make nullptr_t available in the global namespace via stddef.h,
// though it should have according to C++ documentation (see https://en.cppreference.com/w/cpp/types/nullptr_t#Notes).
// actually it should be available when including stddef.h
using std::nullptr_t;

#include "cxx_core_features.h"
#include "cxx_compiler_quirks.h"
