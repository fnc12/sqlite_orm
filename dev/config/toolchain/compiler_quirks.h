#pragma once

/*
 *  The workarounds for compiler quirks sqlite_orm depends on.
 */

#include "compiler.h"
#include "language_features.h"

#if defined(SQLITE_ORM_MS_MSVC) && (_MSC_VER < 1920)
#define SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
// Type replacement may fail if an alias template has a non-type template parameter from a dependent expression in it,
// `e.g. template<class T> using is_something = std::bool_constant<is_something_v<T>>;`
// Remedies:
// a. use a derived struct: `template<class T> struct is_something : std::bool_constant<is_something_v<T>>;`
// b. hoist the result into a constexpr value: `static constexpr auto value = is_something_v<T>; using is_something : std::bool_constant<value>;`
#define SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR
#endif

// These compilers are known to have problems with alias templates in SFINAE contexts:
// clang 3.5
// gcc 8.3
// msvc 15.9
// Type replacement may fail if an alias template has dependent expression or decltype in it.
// In these cases we have to use helper structures to break down the type alias.
// Note that the detection of specific compilers is so complicated because some compilers emulate other compilers,
// so we simply exclude all compilers that do not support C++20, even though this test is actually inaccurate.
#if (defined(SQLITE_ORM_MS_MSVC) && (_MSC_VER < 1920)) || (!defined(SQLITE_ORM_MS_MSVC) && (__cplusplus < 202002L))
#define SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_EXPR_SFINAE
#endif

// msvc 16.11 in C++20 mode fails to look up a type alias declared in an enclosing lambda if the use site is
// inside a lambda nested in that lambda; a single lambda level is fine.
// Depending on the complexity of the translation unit this surfaces either as C2653, or as an internal compiler
// error reported at the end of the translation unit without a source location.
// Remedy: redeclare the alias in the generic lambda that uses it.
// Verified broken with 19.29, verified fixed with 19.44 and 19.51; the versions in between were not tested.
#if defined(SQLITE_ORM_MS_MSVC) && (_MSC_VER < 1944)
#define SQLITE_ORM_BROKEN_NESTED_LAMBDA_SCOPE_LOOKUP
#endif

// clang 10 chokes on concepts that don't depend on template parameters;
// when it tries to instantiate an expression in a requires expression, which results in an error,
// the compiler reports an error instead of dismissing the templated function.
#if defined(SQLITE_ORM_CONCEPTS_SUPPORTED) && (defined(__clang__) && (__clang_major__ == 10))
#define SQLITE_ORM_BROKEN_NONTEMPLATE_CONCEPTS
#endif

#if defined(__clang__) && (__clang_major__ <= 15)
#define SQLITE_ORM_BROKEN_CPP20_VIEWS
#endif

// gcc 11 and 12 relocate bytewise a temporary that lives across a suspension point of a `co_await` expression,
// which breaks any type with a non-trivial move constructor, e.g. a `std::string` in its small-buffer state
// in `co_await storage.insert(User{...})`.
// Verified broken with 11.4 and 12.5, verified fixed with 13.4.
// gcc 11 also initializes a pack init-capture bytewise and relocates bytewise a closure passed as a coroutine
// parameter, with the same effect on captured strings; `bind_arguments()` and `async_storage::run()` in
// `execution/async_storage.h` avoid both, with a tuple and by moving the closure to the heap.
#if defined(SQLITE_ORM_GNU_GCC) && (__GNUC__ < 13)
#define SQLITE_ORM_BROKEN_COROUTINE_TEMPORARIES
#endif
