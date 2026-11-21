#pragma once

/** @file
 *  Purpose: gather the public configuration, macros only.
 *
 *  Intention: gathers one header per tier:
 *  - `toolchain/language_features.h`, the C++ language features;
 *  - `toolchain/stdlib_features.h`, the C++ standard library features;
 *  - `toolchain/compiler.h` and `toolchain/platform.h`, the compiler and the platform;
 *  - `sqlite3/sqlite3_config.h`, the SQLite features;
 *  - `capabilities.h`, sqlite_orm's capabilities;
 *  - `toolchain/compiler_quirks.h`, the internal tier: published only because the tiers above rest on it, and subject
 *    to change without notice.
 *  `internal_macros.h` builds sqlite_orm's own spelling on the tiers, and `toolchain/check_prerequisites.h`, outside
 *  them, rejects a toolchain short of C++17.
 *
 *  Published as <sqlite_orm/sqlite_orm_config.h> for consumers of the named module, which cannot export macros:
 *  include it ahead of `import sqlite_orm;` and `import std;`.
 *  The standard permits either order, but GCC rejects including it after either import: it does not merge
 *  the standard library's inline definitions from `<version>` with those reachable from the module
 *  [see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99000].
 *  The include guard is shared by both published headers, which each contain this header.
 */

#ifndef SQLITE_ORM_CONFIG_INCLUDED
#define SQLITE_ORM_CONFIG_INCLUDED

// #include "capabilities.h"

/** @file
 *  Purpose: tell which optional features sqlite_orm offers with the C++ language, standard library, platform and
 *  SQLite in use.
 *
 *  Intention: the capabilities tier of the configuration gathered by `config.h`, built on the other tiers;
 *  sqlite_orm's headers and its consumers test `SQLITE_ORM_WITH_*` for a feature.
 */

// #include "../sqlite3/sqlite3_config.h"

/** @file
 *  Purpose: tell which optional SQLite features the SQLite in use provides.
 *
 *  Intention: the SQLite tier of the configuration gathered by `config.h`, read from `<sqlite3.h>`'s version and the
 *  `SQLITE_ENABLE_*` and `SQLITE_OMIT_*` macros a distribution defines; sqlite_orm's headers test
 *  `SQLITE_ORM_*_SUPPORTED` to offer the functions and tables of a feature.
 */

#include <sqlite3.h>

/*
 *  JSON functions are built into SQLite as of 3.38.0, unless it was built with `SQLITE_OMIT_JSON`.
 *  Before that they had to be requested with `SQLITE_ENABLE_JSON1`, which is a no-op since - which is
 *  why a distribution that has JSON need not define it. vcpkg, for one, reports the feature the modern
 *  way: its `sqlite3-vcpkg-config.h` leaves `SQLITE_OMIT_JSON` undefined and never mentions JSON1.
 *
 *  A distribution that omits JSON therefore has to say so in a header the consumer sees, as vcpkg does
 *  for the features it selects; otherwise sqlite_orm cannot tell and the functions fail to link.
 */
#if defined(SQLITE_ENABLE_JSON1) || (SQLITE_VERSION_NUMBER >= 3038000 && !defined(SQLITE_OMIT_JSON))
#define SQLITE_ORM_JSON_SUPPORTED
#endif

/*
 *  The extension loading API is compiled in unless SQLite was built with `SQLITE_OMIT_LOAD_EXTENSION`.
 *  Apple's system SQLite is such a build, and it also strips `sqlite3_load_extension` from its header
 *  without defining the omit macro, leaving nothing for the preprocessor to test for. Hence extension
 *  loading is off on Apple platforms by default; when building against an unrestricted SQLite there
 *  (e.g. from Homebrew or vcpkg), request it by defining `SQLITE_ORM_ENABLE_LOAD_EXTENSION`.
 */
#if !defined(SQLITE_OMIT_LOAD_EXTENSION) && (!defined(__APPLE__) || defined(SQLITE_ORM_ENABLE_LOAD_EXTENSION))
#define SQLITE_ORM_LOAD_EXTENSION_SUPPORTED
#endif

/*
 *  FTS5 is part of the amalgamation as of SQLite 3.9.0, compiled in by default by the amalgamation's configure script,
 *  but not by the source tree's; other builds request it with `SQLITE_ENABLE_FTS5` [https://sqlite.org/fts5.html].
 *  Since a distribution need not define the latter in a header the consumer sees, sqlite_orm assumes FTS5 as of 3.9.0.
 */
#if defined(SQLITE_ENABLE_FTS5) || SQLITE_VERSION_NUMBER >= 3009000
#define SQLITE_ORM_FTS5_SUPPORTED
#endif

/*
 *  The math functions are part of the amalgamation as of SQLite 3.35.0, compiled in by default by the configure script
 *  on unix and by `Makefile.msc` on Windows; other builds request them with `SQLITE_ENABLE_MATH_FUNCTIONS`
 *  [https://sqlite.org/compile.html]. As with FTS5, sqlite_orm assumes them as of 3.35.0.
 */
#if defined(SQLITE_ENABLE_MATH_FUNCTIONS) || SQLITE_VERSION_NUMBER >= 3035000
#define SQLITE_ORM_MATH_FUNCTIONS_SUPPORTED
#endif

/*
 *  The features below are compiled into SQLite only on request, so sqlite_orm offers them only if the requesting macro
 *  is visible to it, e.g. through vcpkg's `sqlite3-vcpkg-config.h`, or defined by the project.
 */
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
#define SQLITE_ORM_DBSTAT_SUPPORTED
#endif

#ifdef SQLITE_ENABLE_RTREE
#define SQLITE_ORM_RTREE_SUPPORTED
#endif

#ifdef SQLITE_ENABLE_PERCENTILE
#define SQLITE_ORM_PERCENTILE_SUPPORTED
#endif

#ifdef SQLITE_ENABLE_OFFSET_SQL_FUNC
#define SQLITE_ORM_OFFSET_SQL_FUNC_SUPPORTED
#endif

#ifdef SQLITE_SOUNDEX
#define SQLITE_ORM_SOUNDEX_SUPPORTED
#endif

//  `sqlite3_normalized_sql()` exists as of SQLite 3.26.0
#if SQLITE_VERSION_NUMBER >= 3026000 && defined(SQLITE_ENABLE_NORMALIZE)
#define SQLITE_ORM_NORMALIZE_SUPPORTED
#endif

// #include "toolchain/language_features.h"

/** @file
 *  Purpose: detect the C++ core language features sqlite_orm can use.
 *
 *  Intention: the language tier of the configuration gathered by `config.h`, read from the feature-test macros
 *  `__cpp_*` and from `__cplusplus`, and amended where a compiler supports a feature it does not report;
 *  `compiler_quirks.h`, `stdlib_features.h` and `capabilities.h` build on it. Its macros are named
 *  `SQLITE_ORM_CPP_<feature>_SUPPORTED`, after the standard's prefix `__cpp_`.
 */

#if __cpp_generic_lambdas >= 201707L
#define SQLITE_ORM_CPP_EXPLICIT_GENERIC_LAMBDA_SUPPORTED
#endif

#if __cpp_init_captures >= 201803L
#define SQLITE_ORM_CPP_PACK_EXPANSION_IN_INIT_CAPTURE_SUPPORTED
#endif

#if __cpp_consteval >= 201811L
#define SQLITE_ORM_CPP_CONSTEVAL_SUPPORTED
#endif

#if __cpp_char8_t >= 201811L
#define SQLITE_ORM_CPP_CHAR8T_SUPPORTED
#endif

#if __cpp_aggregate_paren_init >= 201902L
#define SQLITE_ORM_CPP_AGGREGATE_PAREN_INIT_SUPPORTED
#endif

#if __cpp_constexpr >= 201907L
#define SQLITE_ORM_CPP_TRIVIAL_DEFAULTINIT_SUPPORTED
#endif

#if __cpp_concepts >= 201907L
#define SQLITE_ORM_CPP_CONCEPTS_SUPPORTED
#endif

// defaulted comparisons have no feature-test macro of their own; the standard reports them with three-way comparison
#if __cpp_impl_three_way_comparison >= 201907L
#define SQLITE_ORM_CPP_DEFAULT_COMPARISONS_SUPPORTED
#endif

#if __cpp_nontype_template_args >= 201911L
#define SQLITE_ORM_CPP_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

// overwrite SQLITE_ORM_CPP_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#if (__cpp_nontype_template_args < 201911L) &&                                                                         \
    (defined(__clang__) && (__clang_major__ >= 12) && (__cplusplus >= 202002L))
#define SQLITE_ORM_CPP_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

#if __cpp_explicit_this_parameter >= 202110L
#define SQLITE_ORM_CPP_DEDUCING_THIS_SUPPORTED
#endif

#if __cpp_static_call_operator >= 202207L
#define SQLITE_ORM_CPP_STATIC_CALL_OPERATOR_SUPPORTED
#endif

#if __cpp_pack_indexing >= 202311L
#define SQLITE_ORM_CPP_PACK_INDEXING_SUPPORTED
#endif

#if __cpp_structured_bindings >= 202411L
#define SQLITE_ORM_CPP_STRUCTURED_BINDING_PACK_SUPPORTED
#endif

#if __cpp_contracts >= 202502L
#define SQLITE_ORM_CPP_CONTRACTS_SUPPORTED
#endif

#if __cpp_impl_reflection >= 202506L
#define SQLITE_ORM_CPP_REFLECTION_SUPPORTED
#endif

#if __cplusplus >= 202002L
#define SQLITE_ORM_CPP_INITSTMT_RANGE_BASED_FOR_SUPPORTED
#endif

// #include "toolchain/platform.h"

/** @file
 *  Purpose: identify the platform sqlite_orm is built for.
 *
 *  Intention: the platform tier of the configuration gathered by `config.h`; `capabilities.h` builds on it. Its macros
 *  are named `SQLITE_ORM_PLATFORM_<platform>`; `SQLITE_ORM_PLATFORM_UNIX` accompanies each platform but Windows, and an
 *  unknown platform stops the build.
 */

#if defined(_WIN32)
#define SQLITE_ORM_PLATFORM_WIN

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#define SQLITE_ORM_PLATFORM_IOS
#elif TARGET_OS_OSX == 1
#define SQLITE_ORM_PLATFORM_MACOS
#endif
#define SQLITE_ORM_PLATFORM_APPLE
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__linux__)
#if defined(__ANDROID__)
#define SQLITE_ORM_PLATFORM_ANDROID
#endif
#define SQLITE_ORM_PLATFORM_LINUX
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define SQLITE_ORM_PLATFORM_BSD
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__RTP__) || defined(_WRS_KERNEL)
#define SQLITE_ORM_PLATFORM_VXWORKS
#define SQLITE_ORM_PLATFORM_UNIX

#elif defined(__unix__) || defined(__unix)
#define SQLITE_ORM_PLATFORM_UNIX

#else
#error "Unknown target platform detected"
#endif

// #include "toolchain/stdlib_features.h"

/** @file
 *  Purpose: detect the C++ standard library features sqlite_orm can use.
 *
 *  Intention: the standard library tier of the configuration gathered by `config.h`, read from the feature-test macros
 *  `__cpp_lib_*` of `<version>`, and from the language features and compiler quirks a library feature needs;
 *  `capabilities.h` builds on it. Its macros are named `SQLITE_ORM_CPPxx_LIB_<feature>_SUPPORTED`, after the
 *  standard's prefix `__cpp_lib_` and the standard the feature came with.
 */

// #include "compiler_quirks.h"

/** @file
 *  Purpose: work around the compiler defects sqlite_orm's own code meets.
 *
 *  Intention: the internal tier of the configuration gathered by `config.h`: published only because
 *  `stdlib_features.h` and `capabilities.h` rest on it, and subject to change without notice. Keyed on `compiler.h` and
 *  `language_features.h`; sqlite_orm's headers test `SQLITE_ORM_INTERNAL_BROKEN_*` to choose a workaround.
 */

// #include "compiler.h"

/** @file
 *  Purpose: identify the compiler that translates sqlite_orm.
 *
 *  Intention: the compiler tier of the configuration gathered by `config.h`; `compiler_quirks.h` keys its workarounds
 *  on it. Its macros are named `SQLITE_ORM_COMPILER_<maker>_<compiler>`, the compiler being the one it presents itself
 *  as: `SQLITE_ORM_COMPILER_GNU_GCC` is GNU's own GCC, as Clang defines `__GNUC__` too, and
 *  `SQLITE_ORM_COMPILER_CLANG_MSVC` is Clang presenting itself as MSVC.
 */

#if defined(_MSC_VER) && !defined(__clang__)
#define SQLITE_ORM_COMPILER_MS_MSVC
#endif

#if defined(__clang__) && defined(_MSC_VER)
#define SQLITE_ORM_COMPILER_CLANG_MSVC
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define SQLITE_ORM_COMPILER_GNU_GCC
#endif

// #include "language_features.h"

#if defined(SQLITE_ORM_COMPILER_MS_MSVC) && (_MSC_VER < 1920)
#define SQLITE_ORM_INTERNAL_BROKEN_VARIADIC_PACK_EXPANSION
// Type replacement may fail if an alias template has a non-type template parameter from a dependent expression in it,
// `e.g. template<class T> using is_something = std::bool_constant<is_something_v<T>>;`
// Remedies:
// a. use a derived struct: `template<class T> struct is_something : std::bool_constant<is_something_v<T>>;`
// b. hoist the result into a constexpr value: `static constexpr auto value = is_something_v<T>; using is_something : std::bool_constant<value>;`
#define SQLITE_ORM_INTERNAL_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR
#endif

// These compilers are known to have problems with alias templates in SFINAE contexts:
// clang 3.5
// gcc 8.3
// msvc 15.9
// Type replacement may fail if an alias template has dependent expression or decltype in it.
// In these cases we have to use helper structures to break down the type alias.
// Note that the detection of specific compilers is so complicated because some compilers emulate other compilers,
// so we simply exclude all compilers that do not support C++20, even though this test is actually inaccurate.
#if (defined(SQLITE_ORM_COMPILER_MS_MSVC) && (_MSC_VER < 1920)) ||                                                     \
    (!defined(SQLITE_ORM_COMPILER_MS_MSVC) && (__cplusplus < 202002L))
#define SQLITE_ORM_INTERNAL_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE
#endif

// msvc 16.11 in C++20 mode fails to look up a type alias declared in an enclosing lambda if the use site is
// inside a lambda nested in that lambda; a single lambda level is fine.
// Depending on the complexity of the translation unit this surfaces either as C2653, or as an internal compiler
// error reported at the end of the translation unit without a source location.
// Remedy: redeclare the alias in the generic lambda that uses it.
// Verified broken with 19.29, verified fixed with 19.44 and 19.51; the versions in between were not tested.
#if defined(SQLITE_ORM_COMPILER_MS_MSVC) && (_MSC_VER < 1944)
#define SQLITE_ORM_INTERNAL_BROKEN_NESTED_LAMBDA_SCOPE_LOOKUP
#endif

// clang 10 chokes on concepts that don't depend on template parameters;
// when it tries to instantiate an expression in a requires expression, which results in an error,
// the compiler reports an error instead of dismissing the templated function.
#if defined(SQLITE_ORM_CPP_CONCEPTS_SUPPORTED) && (defined(__clang__) && (__clang_major__ == 10))
#define SQLITE_ORM_INTERNAL_BROKEN_NONTEMPLATE_CONCEPTS
#endif

#if defined(__clang__) && (__clang_major__ <= 15)
#define SQLITE_ORM_INTERNAL_BROKEN_CPP20_VIEWS
#endif

// gcc 11 and 12 relocate bytewise a temporary that lives across a suspension point of a `co_await` expression,
// which breaks any type with a non-trivial move constructor, e.g. a `std::string` in its small-buffer state
// in `co_await storage.insert(User{...})`.
// Verified broken with 11.4 and 12.5, verified fixed with 13.4.
// gcc 11 also initializes a pack init-capture bytewise and relocates bytewise a closure passed as a coroutine
// parameter, with the same effect on captured strings; `bind_arguments()` and `async_storage::run()` in
// `execution/async_storage.h` avoid both, with a tuple and by moving the closure to the heap.
#if defined(SQLITE_ORM_COMPILER_GNU_GCC) && (__GNUC__ < 13)
#define SQLITE_ORM_INTERNAL_BROKEN_COROUTINE_TEMPORARIES
#endif

// #include "language_features.h"

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
#define SQLITE_ORM_CPP20_LIB_IS_CONSTANT_EVALUATED_SUPPORTED
#endif

#if defined(SQLITE_ORM_CPP_CONCEPTS_SUPPORTED) && __cpp_lib_concepts >= 202002L
#define SQLITE_ORM_CPP20_LIB_CONCEPTS_SUPPORTED
#endif

#if __cpp_lib_ranges >= 201911L
#define SQLITE_ORM_CPP20_LIB_RANGES_SUPPORTED
#endif

#if __cpp_lib_ranges >= 202110L && !defined(SQLITE_ORM_INTERNAL_BROKEN_CPP20_VIEWS)
#define SQLITE_ORM_CPP20_LIB_VIEWS_SUPPORTED
#endif

#if __cpp_impl_coroutine >= 201902L && __cpp_lib_coroutine >= 201902L
#define SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED
#endif

#if __cpp_lib_generator >= 202207L
#define SQLITE_ORM_CPP23_LIB_GENERATOR_SUPPORTED
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
#define SQLITE_ORM_CPP20_LIB_DEFAULT_SENTINEL_SUPPORTED
#endif

#if (defined(SQLITE_ORM_CPP_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED) && defined(SQLITE_ORM_CPP_CONSTEVAL_SUPPORTED)) &&      \
    (defined(SQLITE_ORM_CPP20_LIB_CONCEPTS_SUPPORTED))
#define SQLITE_ORM_WITH_CPP20_ALIASES
#endif

#define SQLITE_ORM_WITH_CTE

// the deprecated spelling of `SQLITE_ORM_OMIT_CODECVT`, which omits the `std::wstring` support
#if defined(SQLITE_ORM_OMITS_CODECVT) && !defined(SQLITE_ORM_OMIT_CODECVT)
#define SQLITE_ORM_OMIT_CODECVT
#endif

#if defined(SQLITE_ORM_CPP_REFLECTION_SUPPORTED)
#define SQLITE_ORM_WITH_VIEW
#endif

// The asynchronous storage: its I/O engine is io_uring (Linux), its fibers switch stacks with inline assembly
// written for GCC and Clang on x86_64 and aarch64.
#if (defined(SQLITE_ORM_PLATFORM_LINUX) && !defined(SQLITE_ORM_PLATFORM_ANDROID)) &&                                   \
    (defined(SQLITE_ORM_CPP20_LIB_COROUTINES_SUPPORTED) &&                                                             \
     !defined(SQLITE_ORM_INTERNAL_BROKEN_COROUTINE_TEMPORARIES)) &&                                                    \
    (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__aarch64__))
#define SQLITE_ORM_WITH_ASYNC
#endif

// #include "toolchain/compiler.h"

// #include "toolchain/compiler_quirks.h"

// #include "toolchain/language_features.h"

// #include "toolchain/platform.h"

// #include "toolchain/stdlib_features.h"

#endif
