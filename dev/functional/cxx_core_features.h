#pragma once

/*
 *  This header detects core C++ language features.
 *  May be updated/overwritten by cxx_compiler_quirks.h
 */

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

#if __cpp_noexcept_function_type >= 201510L
#define SQLITE_ORM_NOTHROW_ALIASES_SUPPORTED
#endif

#if __cpp_aggregate_bases >= 201603L
#define SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
#endif

#if __cpp_fold_expressions >= 201603L
#define SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
#endif

#if __cpp_constexpr >= 201603L
#define SQLITE_ORM_CONSTEXPR_LAMBDAS_SUPPORTED
#endif

#if __cpp_range_based_for >= 201603L
#define SQLITE_ORM_SENTINEL_BASED_FOR_SUPPORTED
#endif

#if __cpp_if_constexpr >= 201606L
#define SQLITE_ORM_IF_CONSTEXPR_SUPPORTED
#endif

#if __cpp_inline_variables >= 201606L
#define SQLITE_ORM_INLINE_VARIABLES_SUPPORTED
#endif

#if __cpp_structured_bindings >= 201606L
#define SQLITE_ORM_STRUCTURED_BINDINGS_SUPPORTED
#endif

#if __cpp_aligned_new >= 201606L
#define SQLITE_ORM_ALIGNED_NEW_SUPPORTED
#endif

#if __cpp_deduction_guides >= 201703L
#define SQLITE_ORM_CTAD_SUPPORTED
#endif

#if __cpp_generic_lambdas >= 201707L
#define SQLITE_ORM_EXPLICIT_GENERIC_LAMBDA_SUPPORTED
#endif

#if __cpp_init_captures >= 201803L
#define SQLITE_ORM_PACK_EXPANSION_IN_INIT_CAPTURE_SUPPORTED
#endif

#if __cpp_consteval >= 201811L
#define SQLITE_ORM_CONSTEVAL_SUPPORTED
#endif

#if __cpp_char8_t >= 201811L
#define SQLITE_ORM_CHAR8T_SUPPORTED
#endif

#if __cpp_aggregate_paren_init >= 201902L
#define SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
#endif

#if __cpp_concepts >= 201907L
#define SQLITE_ORM_CONCEPTS_SUPPORTED
#endif

#if __cpp_nontype_template_args >= 201911L
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

#if __cpp_nontype_template_args >= 201911L
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

#if __cpp_pack_indexing >= 202311L
#define SQLITE_ORM_PACK_INDEXING_SUPPORTED
#endif

#if __cplusplus >= 202002L
#define SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
#define SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
#endif
