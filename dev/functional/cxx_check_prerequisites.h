#pragma once

/*
 *  This header detects missing core C++ language features on which sqlite_orm depends, bailing out with a hard error.
 */

// note: Before the C++17 language standard was made the baseline, the library had workarounds for these specific missing C++14 language features,
// so they are kept here as explicit checks for reference.
#if __cpp_aggregate_nsdmi < 201304L || __cpp_constexpr < 201304L
#error A fully C++17-compliant compiler is required.
#endif

#if (__cpp_noexcept_function_type < 201510L) ||                                                                        \
    (__cpp_fold_expressions < 201603L || __cpp_constexpr < 201603L || __cpp_aggregate_bases < 201603L ||               \
     __cpp_range_based_for < 201603L) ||                                                                               \
    (__cpp_if_constexpr < 201606L || __cpp_inline_variables < 201606L || __cpp_structured_bindings < 201606L)
#error A fully C++17-compliant compiler is required.
#endif
