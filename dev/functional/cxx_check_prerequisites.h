#pragma once

/*
 *  This header detects missing core C++ language features and standard library features
 *  on which sqlite_orm depends, bailing out with a hard error.
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

// Make the standard library's feature-test macros universally available:
// `<version>` is a C++20 header, but is shipped independently of the language mode by libstdc++ 9, libc++ 8 and MSVC STL 19.20 or newer.
// Older standard libraries only know the pre-C++20 idiom of including `<ciso646>`, an otherwise empty header
// that drags in the standard library's configuration. Note that this is what keeps MSVC 19.16 (VC 2017 15.9) viable,
// since it is the only one of the three that defines the feature-test macros in its configuration.
#if __has_include(<version>)
#include <version>
#elif __has_include(<ciso646>)
#include <ciso646>
#endif

// note: The check is deliberately limited to the C++17 library features sqlite_orm relies on;
// e.g. `__cpp_lib_hardware_interference_size` is a C++17 feature that libc++ still does not provide.
#if ((__cpp_lib_void_t < 201411L || __cpp_lib_bool_constant < 201505L) ||                                              \
     (__cpp_lib_logical_traits < 201510L || __cpp_lib_type_trait_variable_templates < 201510L) ||                      \
     (__cpp_lib_is_aggregate < 201703L)) ||                                                                            \
    ((__cpp_lib_invoke < 201411L || __cpp_lib_apply < 201603L || __cpp_lib_is_invocable < 201703L)) ||                 \
    ((__cpp_lib_optional < 201606L || __cpp_lib_string_view < 201606L))
#error A fully C++17-compliant standard library is required (libstdc++ 9, libc++ 8, MSVC 19.16 or newer).
#endif
