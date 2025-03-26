#pragma once

/*
 *  This header detects missing core C++ language features on which sqlite_orm depends, bailing out with a hard error.
 */

#if __cpp_aggregate_nsdmi < 201304L || __cpp_constexpr < 201304L
#error A fully C++17-compliant compiler is required.
#endif

#if (__cpp_noexcept_function_type < 201510L) ||                                                                        \
    (__cpp_fold_expressions < 201603L || __cpp_constexpr < 201603L || __cpp_aggregate_bases < 201603L) ||              \
    (__cpp_if_constexpr < 201606L)
#error A fully C++17-compliant compiler is required.
#endif
