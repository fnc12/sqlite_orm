#pragma once

/*
 *  This header detects missing core C++ language features on which sqlite_orm depends, bailing out with a hard error.
 */

#if __cpp_aggregate_nsdmi < 201304L
#error A fully C++14-compliant compiler is required.
#endif

#if __cpp_constexpr < 201304L
#error A fully C++14-compliant compiler is required.
#endif
