#pragma once

#ifdef __clang__
#pragma clang diagnostic push
// Clang has the annoying habit of warning about future C++ features that it claims to support through a feature macro.
#pragma clang diagnostic ignored "-Wc++20-extensions"
#pragma clang diagnostic ignored "-Wc++23-extensions"
#pragma clang diagnostic ignored "-Wc++26-extensions"

// clang has the bad habit of diagnosing missing brace-init-lists when constructing aggregates with base classes.
// This is a false positive, since the C++ standard is quite clear that braces for nested or base objects may be omitted,
// see https://en.cppreference.com/w/cpp/language/aggregate_initialization:
// "The braces around the nested initializer lists may be elided (omitted),
//  in which case as many initializer clauses as necessary are used to initialize every member or element of the corresponding subaggregate,
//  and the subsequent initializer clauses are used to initialize the following members of the object."
// In this sense clang should only warn about missing field initializers.
// Because we know what we are doing, we suppress the diagnostic message
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif  // defined(_MSC_VER)
