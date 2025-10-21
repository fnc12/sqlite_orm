#pragma once

// Clang has the annoying habit of warning about future C++ features that it claims to support through a feature macro.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++20-extensions"
#pragma clang diagnostic ignored "-Wc++23-extensions"
#pragma clang diagnostic ignored "-Wc++26-extensions"
#endif

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif  // defined(_MSC_VER)
