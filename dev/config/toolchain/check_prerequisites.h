#pragma once

/** @file
 *  Purpose: reject a compiler or standard library that lacks the C++17 features sqlite_orm depends on.
 *
 *  Intention: included by both variants of <sqlite_orm/sqlite_orm.h> ahead of the configuration, and left out of the
 *  published <sqlite_orm/sqlite_orm_config.h>.
 */

// note: Before the C++17 language standard was made the baseline, the library had workarounds for these specific missing C++14 language features,
// so they are kept here as explicit checks for reference.
#if (__cpp_aggregate_nsdmi < 201304L || __cpp_constexpr < 201304L)
#error A fully C++17-compliant compiler is required.
#endif

#if (!defined(__has_include)) ||                                                                                       \
    ((__cpp_static_assert < 201411L) || (__cpp_noexcept_function_type < 201510L) ||                                    \
     (__cpp_fold_expressions < 201603L || __cpp_constexpr < 201603L || __cpp_aggregate_bases < 201603L ||              \
      __cpp_range_based_for < 201603L) ||                                                                              \
     (__cpp_if_constexpr < 201606L || __cpp_inline_variables < 201606L || __cpp_structured_bindings < 201606L) ||      \
     (__cpp_deduction_guides < 201703L))
#error A fully C++17-compliant compiler is required.
#endif

// on purpose: a copy of the `<version>` include of stdlib_features.h - the fallback goes once C++20 is the baseline
#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#elif __has_include(<ciso646>)
#include <ciso646>
#endif
#endif

// note: The check is deliberately limited to the C++17 library features sqlite_orm relies on;
// e.g. `__cpp_lib_hardware_interference_size` is a C++17 feature that libc++ still does not provide.
// It also rules out libstdc++ 7-8 and libc++ 7, which provide these features but no populated `<version>`
// (libc++ 7's is an empty stub). Testing the feature headers themselves instead would include the standard library
// ahead of everything, which conflicts with the `SQLITE_ORM_IMPORT_STD_MODULE` build.
// The Apple deployment target (macOS 10.14 / iOS 12 for `std::optional`) is beyond any macro test.
#if ((__cpp_lib_void_t < 201411L || __cpp_lib_bool_constant < 201505L) ||                                              \
     (__cpp_lib_logical_traits < 201510L || __cpp_lib_type_trait_variable_templates < 201510L) ||                      \
     (__cpp_lib_is_aggregate < 201703L)) ||                                                                            \
    ((__cpp_lib_invoke < 201411L || __cpp_lib_apply < 201603L || __cpp_lib_is_invocable < 201703L)) ||                 \
    ((__cpp_lib_optional < 201606L || __cpp_lib_string_view < 201606L))
#error A fully C++17-compliant standard library is required (libstdc++ 9, libc++ 8, MSVC 19.16 or newer).
#endif
