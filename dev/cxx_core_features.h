#pragma once

#ifdef __has_cpp_attribute
#define SQLITE_ORM_HAS_CPP_ATTRIBUTE(attr) __has_cpp_attribute(attr)
#else
#define SQLITE_ORM_HAS_CPP_ATTRIBUTE(attr) 0L
#endif

#if __cpp_aggregate_nsdmi >= 201304L
#define SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
#endif

#if __cpp_constexpr >= 201304L
#define SQLITE_ORM_RELAXED_CONSTEXPR
#endif

#if __cpp_noexcept_function_type >= 201510L
#define SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
#endif

#if __cpp_aggregate_bases >= 201603L
#define SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
#endif

#if __cpp_fold_expressions >= 201603L
#define SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
#endif

#if __cpp_inline_variables >= 201606L
#define SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#endif

#if __cpp_if_constexpr >= 201606L
#define SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
#endif

#if __cpp_inline_variables >= 201606L
#define SQLITE_ORM_INLINE_VAR inline
#else
#define SQLITE_ORM_INLINE_VAR
#endif

#if SQLITE_ORM_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif

#if __cpp_consteval >= 201811L
#define SQLITE_ORM_CONSTEVAL consteval
#else
#define SQLITE_ORM_CONSTEVAL constexpr
#endif

#if __cpp_aggregate_paren_init >= 201902L
#define SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
#endif

#if __cpp_concepts >= 201907L
#define SQLITE_ORM_CONCEPTS_SUPPORTED
#endif

#if __cplusplus >= 201703L  // C++17 or later
#if __has_include(<optional>)
#define SQLITE_ORM_OPTIONAL_SUPPORTED
#endif

#if __has_include(<string_view>)
#define SQLITE_ORM_STRING_VIEW_SUPPORTED
#endif
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#define SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
#endif
