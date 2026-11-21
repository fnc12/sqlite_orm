#pragma once

/** @file
 *  Purpose: detect the C++ core language features sqlite_orm can use.
 *
 *  Intention: the language tier of the configuration gathered by `config.h`, read from the feature-test macros
 *  `__cpp_*` and from `__cplusplus`, and amended where a compiler supports a feature it does not report;
 *  `compiler_quirks.h`, `stdlib_features.h` and `capabilities.h` build on it.
 */

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

#if __cpp_constexpr >= 201907L
#define SQLITE_ORM_TRIVIAL_DEFAULTINIT_SUPPORTED
#endif

#if __cpp_concepts >= 201907L
#define SQLITE_ORM_CONCEPTS_SUPPORTED
#endif

#if __cpp_nontype_template_args >= 201911L
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

// overwrite SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#if (__cpp_nontype_template_args < 201911L) &&                                                                         \
    (defined(__clang__) && (__clang_major__ >= 12) && (__cplusplus >= 202002L))
#define SQLITE_ORM_CLASSTYPE_TEMPLATE_ARGS_SUPPORTED
#endif

#if __cpp_explicit_this_parameter >= 202110L
#define SQLITE_ORM_DEDUCING_THIS_SUPPORTED
#endif

#if __cpp_static_call_operator >= 202207L
#define SQLITE_ORM_STATIC_CALL_OPERATOR_SUPPORTED
#endif

#if __cpp_pack_indexing >= 202311L
#define SQLITE_ORM_PACK_INDEXING_SUPPORTED
#endif

#if __cpp_structured_bindings >= 202411L
#define SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
#endif

#if __cpp_contracts >= 202502L
#define SQLITE_ORM_CONTRACTS_SUPPORTED
#endif

#if __cpp_impl_reflection >= 202506L
#define SQLITE_ORM_REFLECTION_SUPPORTED
#endif

#if __cplusplus >= 202002L
#define SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
#define SQLITE_ORM_INITSTMT_RANGE_BASED_FOR_SUPPORTED
#endif
