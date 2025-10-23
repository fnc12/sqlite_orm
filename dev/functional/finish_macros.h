#pragma once

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER)
__pragma(pop_macro("max"))
__pragma(pop_macro("min"))
#endif
