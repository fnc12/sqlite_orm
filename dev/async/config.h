#pragma once

/**
 *  Feature detection for the asynchronous storage (dev/async/).
 *
 *  Requirements: Linux (the I/O engine is io_uring, kernel 5.6 or newer, raw
 *  syscalls without liburing), C++20 coroutines, GCC or Clang, x86_64 or aarch64.
 *  Elsewhere the asynchronous part of the library compiles to nothing and
 *  SQLITE_ORM_ASYNC_SUPPORTED stays undefined.
 */
#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#endif
#endif

//  GCC 11 and 12 miscompile temporaries of types with non-trivial move
//  constructors that live across a suspension in a `co_await` expression (they
//  are relocated bytewise), which breaks ordinary user code such as
//  `co_await storage.insert(User{...})`. GCC 13 is the minimum.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 13
#define SQLITE_ORM_ASYNC_UNSUPPORTED_COMPILER
#endif

#if defined(__linux__) && defined(__cpp_impl_coroutine) && defined(__cpp_lib_coroutine) &&                             \
    (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__aarch64__)) &&                      \
    !defined(SQLITE_ORM_ASYNC_UNSUPPORTED_COMPILER)
#define SQLITE_ORM_ASYNC_SUPPORTED
#endif
