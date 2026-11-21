#pragma once

/** @file
 *  Purpose: detect the C++ standard library features sqlite_orm can use.
 *
 *  Intention: the standard library tier of the configuration gathered by `config.h`, read from the feature-test macros
 *  `__cpp_lib_*` of `<version>`, and from the language features and compiler quirks a library feature needs;
 *  `capabilities.h` builds on it.
 */

#include "compiler_quirks.h"
#include "language_features.h"

// Make the standard library's feature-test macros universally available:
// `<version>` is a C++20 header, but is shipped independently of the language mode by libstdc++ 9, libc++ 8 and MSVC STL 19.20 or newer.
// Older standard libraries only know the pre-C++20 idiom of including `<ciso646>`, an otherwise empty header
// that drags in the standard library's configuration. Note that this is what keeps MSVC 19.16 (VC 2017 15.9) viable,
// since it is the only one of the three that defines the feature-test macros in its configuration.
#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#elif __has_include(<ciso646>)
#include <ciso646>
#endif
#endif

#if __cpp_lib_is_constant_evaluated >= 201811L
#define SQLITE_ORM_CPP20_IS_CONSTANT_EVALUATED_SUPPORTED
#endif

#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && __cpp_lib_concepts >= 202002L
#define SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#endif

#if __cpp_lib_ranges >= 201911L
#define SQLITE_ORM_CPP20_RANGES_SUPPORTED
#endif

#if __cpp_lib_ranges >= 202110L && !defined(SQLITE_ORM_INTERNAL_BROKEN_CPP20_VIEWS)
#define SQLITE_ORM_CPP20_VIEWS_SUPPORTED
#endif

#if __cpp_impl_coroutine >= 201902L && __cpp_lib_coroutine >= 201902L
#define SQLITE_ORM_CPP20_COROUTINES_SUPPORTED
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
