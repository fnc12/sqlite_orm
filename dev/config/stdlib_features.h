#pragma once

/*
 *  Detection of the C++ standard library features.
 */

#include "prerequisites.h"
#include "language_features.h"
#include "compiler_quirks.h"

#if __cpp_lib_is_constant_evaluated >= 201811L
#define SQLITE_ORM_CPP20_IS_CONSTANT_EVALUATED_SUPPORTED
#endif

#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && __cpp_lib_concepts >= 202002L
#define SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#endif

#if __cpp_lib_ranges >= 201911L
#define SQLITE_ORM_CPP20_RANGES_SUPPORTED
#endif

#if __cpp_lib_ranges >= 202110L && !defined(SQLITE_ORM_BROKEN_CPP20_VIEWS)
#define SQLITE_ORM_CPP20_VIEWS_SUPPORTED
#endif

#if __cpp_lib_generator >= 202207L
#define SQLITE_ORM_CPP23_GENERATOR_SUPPORTED
#endif

// C++20 or later (unfortunately there's no feature test macro).
// Stupidly, clang says C++20, but `std::default_sentinel_t` was only implemented in libc++ 13 and libstd++-v3 10
// (the latter is used on Linux).
// gcc got it right and reports C++20 only starting with v10.
// The check here doesn't care and checks the library versions in use.
//
// Another way of detection might be the feature-test macro __cpp_lib_concepts
#if (__cplusplus >= 202002L) &&                                                                                        \
    ((!_LIBCPP_VERSION || _LIBCPP_VERSION >= 13000) && (!_GLIBCXX_RELEASE || _GLIBCXX_RELEASE >= 10))
#define SQLITE_ORM_STL_HAS_DEFAULT_SENTINEL
#endif
