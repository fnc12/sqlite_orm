#pragma once

#ifdef __has_cpp_attribute
#define SQLITE_ORM_HAS_CPP_ATTRIBUTE(attr) __has_cpp_attribute(attr)
#else
#define SQLITE_ORM_HAS_CPP_ATTRIBUTE(attr) 0L
#endif

#ifdef __has_include
#define SQLITE_ORM_HAS_INCLUDE(file) __has_include(file)
#else
#define SQLITE_ORM_HAS_INCLUDE(file) 0L
#endif

// If possible, use an intrinsic provided by Clang
#if defined(__has_builtin)
#if __has_builtin(__type_pack_element)
#define SQLITE_ORM_HAS_TYPE_PACK_ELEMENT_INTRINSIC
#endif
#endif

#if __cpp_aggregate_nsdmi >= 201304L
#define SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
#endif

#if __cpp_constexpr >= 201304L
#define SQLITE_ORM_RELAXED_CONSTEXPR_SUPPORTED
#define SQLITE_ORM_NONCONST_CONSTEXPR constexpr
#else
#define SQLITE_ORM_NONCONST_CONSTEXPR
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

#if __cpp_generic_lambdas >= 201707L
#define SQLITE_ORM_EXPLICIT_GENERIC_LAMBDA_SUPPORTED
#else
#endif

#if SQLITE_ORM_HAS_CPP_ATTRIBUTE(no_unique_address) >= 201803L
#define SQLITE_ORM_NOUNIQUEADDRESS [[no_unique_address]]
#else
#define SQLITE_ORM_NOUNIQUEADDRESS
#endif

#if __cpp_conditional_explicit >= 201806L
#define SQLITE_ORM_CONDITIONAL_EXPLICIT_SUPPORTED
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
