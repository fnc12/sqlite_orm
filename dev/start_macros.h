#pragma once

#if defined(_MSC_VER)
__pragma(push_macro("min"))
#undef min
__pragma(push_macro("max"))
#undef max
#endif  // defined(_MSC_VER)

#include <iso646.h>  //  alternative operator representations

#if __cpp_noexcept_function_type >= 201510L
#define SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
#endif

#if __cpp_aggregate_bases >= 201603L
#define SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
#endif

#if __cpp_inline_variables >= 201606L
#define SQLITE_ORM_INLINE_VAR inline
#else
#define SQLITE_ORM_INLINE_VAR
#endif

#if __has_cpp_attribute(no_unique_address) >= 201803L
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif

#if __cpp_consteval >= 201811L
#define SQLITE_ORM_CONSTEVAL consteval
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#endif

#if __cpp_nontype_template_args >= 201911
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
#endif

#if __cplusplus >= 201703L  // C++17 or later
#if __has_include(<optional>)
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#endif

#if __has_include(<string_view>)
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#endif
#endif
