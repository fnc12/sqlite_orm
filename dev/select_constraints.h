#pragma once

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>
#endif
#include <type_traits>  //  std::remove_cvref, std::is_convertible, std::is_same, std::is_member_pointer
#include <string>  //  std::string
#include <utility>  //  std::move
#include <tuple>  //  std::tuple, std::get, std::tuple_size
#include "functional/cxx_optional.h"

#include "functional/cxx_type_traits_polyfill.h"
#include "is_base_of_template.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_transformer.h"
#include "tuple_helper/tuple_iteration.h"
#include "optional_container.h"
#include "ast/where.h"
#include "ast/group_by.h"
#include "core_functions.h"
#include "alias_traits.h"
#include "cte_moniker.h"
#include "schema/column.h"

namespace sqlite_orm {

    namespace internal {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        template<class T>
        struct as_optional_t {
            using value_type = T;

            value_type value;
        };
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

        struct distinct_string {
            operator std::string() const {
                return "DISTINCT";
            }
        };

        /**
         *  DISCTINCT generic container.
         */
        template<class T>
        struct distinct_t : distinct_string {
            using value_type = T;

            value_type value;

            distinct_t(value_type value_) : value(std::move(value_)) {}
        };

        struct all_string {
            operator std::string() const {
                return "ALL";
            }
        };

        /**
         *  ALL generic container.
         */
        template<class T>
        struct all_t : all_string {
            T value;

            all_t(T value_) : value(std::move(value_)) {}
        };

        template<class... Args>
        struct columns_t {
            using columns_type = std::tuple<Args...>;

            columns_type columns;
            bool distinct = false;

            static constexpr int count = std::tuple_size<columns_type>::value;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            columns_t(columns_type columns) : columns{std::move(columns)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_columns_v = polyfill::is_specialization_of<T, columns_t>::value;

        template<class T>
        using is_columns = polyfill::bool_constant<is_columns_v<T>>;

        /*
         *  Captures the type of an aggregate/structure/object and column expressions, such that
         *  `T` can be constructed in-place as part of a result row.
         *  `T` must be constructible using direct-list-initialization.
         */
        template<class T, class... Args>
        struct struct_t {
            using columns_type = std::tuple<Args...>;

            columns_type columns;
            bool distinct = false;

            static constexpr int count = std::tuple_size<columns_type>::value;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            struct_t(columns_type columns) : columns{std::move(columns)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_struct_v = polyfill::is_specialization_of<T, struct_t>::value;

        template<class T>
        using is_struct = polyfill::bool_constant<is_struct_v<T>>;

        /**
         *  Subselect object type.
         */
        template<class T, class... Args>
        struct select_t {
            using return_type = T;
            using conditions_type = std::tuple<Args...>;

            return_type col;
            conditions_type conditions;
            bool highest_level = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            select_t(return_type col, conditions_type conditions) :
                col{std::move(col)}, conditions{std::move(conditions)} {}
#endif
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_select_v = polyfill::is_specialization_of<T, select_t>::value;

        template<class T>
        using is_select = polyfill::bool_constant<is_select_v<T>>;

        /**
         *  Base for UNION, UNION ALL, EXCEPT and INTERSECT
         */
        template<class... E>
        struct compound_operator {
            using expressions_tuple = std::tuple<E...>;

            expressions_tuple compound;

            constexpr compound_operator(expressions_tuple compound) : compound{std::move(compound)} {
                iterate_tuple(this->compound, [](auto& expression) {
                    expression.highest_level = true;
                });
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_compound_operator_v = is_base_of_template<T, compound_operator>::value;

        template<class T>
        using is_compound_operator = polyfill::bool_constant<is_compound_operator_v<T>>;

        struct union_base {
            bool all = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            union_base(bool all) : all{all} {}
#endif

            operator std::string() const {
                if(!this->all) {
                    return "UNION";
                } else {
                    return "UNION ALL";
                }
            }
        };

        /**
         *  UNION object type.
         */
        template<class... E>
        struct union_t : public compound_operator<E...>, union_base {
            using typename compound_operator<E...>::expressions_tuple;

            constexpr union_t(expressions_tuple compound, bool all) :
                compound_operator<E...>{std::move(compound)}, union_base{all} {}
        };

        struct except_string {
            operator std::string() const {
                return "EXCEPT";
            }
        };

        /**
         *  EXCEPT object type.
         */
        template<class... E>
        struct except_t : compound_operator<E...>, except_string {
            using super = compound_operator<E...>;

            using super::super;
        };

        struct intersect_string {
            operator std::string() const {
                return "INTERSECT";
            }
        };
        /**
         *  INTERSECT object type.
         */
        template<class... E>
        struct intersect_t : compound_operator<E...>, intersect_string {
            using super = compound_operator<E...>;

            using super::super;
        };

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
        /*
         *  Turn explicit columns for a CTE into types that the CTE backend understands
         */
        template<class T, class SFINAE = void>
        struct decay_explicit_column {
            using type = T;
        };
        template<class T>
        struct decay_explicit_column<T, match_if<is_column_alias, T>> {
            using type = alias_holder<T>;
        };
        template<class T>
        struct decay_explicit_column<T, match_if<std::is_convertible, T, std::string>> {
            using type = std::string;
        };
        template<class T>
        using decay_explicit_column_t = typename decay_explicit_column<T>::type;

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        /*
         *  Materialization hint to instruct SQLite to materialize the select statement of a CTE into an ephemeral table as an "optimization fence".
         */
        struct materialized_t {};

        /*
         *  Materialization hint to instruct SQLite to substitute a CTE's select statement as a subquery subject to optimization.
         */
        struct not_materialized_t {};
#endif

        /**
         *  Monikered (aliased) CTE expression.
         */
        template<class Moniker, class ExplicitCols, class Hints, class Select>
        struct common_table_expression {
            using cte_moniker_type = Moniker;
            using expression_type = Select;
            using explicit_colrefs_tuple = ExplicitCols;
            using hints_tuple = Hints;
            static constexpr size_t explicit_colref_count = std::tuple_size_v<ExplicitCols>;

            SQLITE_ORM_NOUNIQUEADDRESS hints_tuple hints;
            explicit_colrefs_tuple explicitColumns;
            expression_type subselect;

            constexpr common_table_expression(explicit_colrefs_tuple explicitColumns, expression_type subselect) :
                explicitColumns{std::move(explicitColumns)}, subselect{std::move(subselect)} {
                this->subselect.highest_level = true;
            }
        };

        template<class... CTEs>
        using common_table_expressions = std::tuple<CTEs...>;

        template<typename Moniker, class ExplicitCols>
        struct cte_builder {
            ExplicitCols explicitColumns;

#if SQLITE_VERSION_NUMBER >= 3035000 && defined(SQLITE_ORM_WITH_CPP20_ALIASES)
            template<auto... hints, class Select, satisfies<is_select, Select> = true>
            constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<decltype(hints)...>, Select>
            as(Select sel) && {
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<auto... hints, class Compound, satisfies<is_compound_operator, Compound> = true>
            constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<decltype(hints)...>, select_t<Compound>>
            as(Compound sel) && {
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }
#else
            template<class Select, satisfies<is_select, Select> = true>
            constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<>, Select> as(Select sel) && {
                return {std::move(this->explicitColumns), std::move(sel)};
            }

            template<class Compound, satisfies<is_compound_operator, Compound> = true>
            constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<>, select_t<Compound>>
            as(Compound sel) && {
                return {std::move(this->explicitColumns), {std::move(sel)}};
            }
#endif
        };

        /**
         *  WITH object type - expression with prepended CTEs.
         */
        template<class E, class... CTEs>
        struct with_t {
            using cte_type = common_table_expressions<CTEs...>;
            using expression_type = E;

            bool recursiveIndicated;
            cte_type cte;
            expression_type expression;

            with_t(bool recursiveIndicated, cte_type cte, expression_type expression) :
                recursiveIndicated{recursiveIndicated}, cte{std::move(cte)}, expression{std::move(expression)} {
                if constexpr(is_select_v<expression_type>) {
                    this->expression.highest_level = true;
                }
            }
        };
#endif

        /**
         *  Generic way to get DISTINCT value from any type.
         */
        template<class T>
        bool get_distinct(const T&) {
            return false;
        }

        template<class... Args>
        bool get_distinct(const columns_t<Args...>& cols) {
            return cols.distinct;
        }

        template<class T, class... Args>
        bool get_distinct(const struct_t<T, Args...>& cols) {
            return cols.distinct;
        }

        template<class T>
        struct asterisk_t {
            using type = T;

            bool defined_order = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            asterisk_t(bool definedOrder) : defined_order{definedOrder} {}
#endif
        };

        template<class T>
        struct object_t {
            using type = T;

            bool defined_order = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            object_t(bool definedOrder) : defined_order{definedOrder} {}
#endif
        };

        template<class T>
        struct then_t {
            using expression_type = T;

            expression_type expression;
        };

        template<class R, class T, class E, class... Args>
        struct simple_case_t {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;

            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;
        };

        /**
         *  T is a case expression type
         *  E is else type (void is ELSE is omitted)
         *  Args... is a pack of WHEN expressions
         */
        template<class R, class T, class E, class... Args>
        struct simple_case_builder {
            using return_type = R;
            using case_expression_type = T;
            using args_type = std::tuple<Args...>;
            using else_expression_type = E;

            optional_container<case_expression_type> case_expression;
            args_type args;
            optional_container<else_expression_type> else_expression;

            template<class W, class Th>
            simple_case_builder<R, T, E, Args..., std::pair<W, Th>> when(W w, then_t<Th> t) {
                using result_args_type = std::tuple<Args..., std::pair<W, Th>>;
                std::pair<W, Th> newPair{std::move(w), std::move(t.expression)};
                result_args_type result_args = std::tuple_cat(std::move(this->args), std::make_tuple(newPair));
                std::get<std::tuple_size<result_args_type>::value - 1>(result_args) = std::move(newPair);
                return {std::move(this->case_expression), std::move(result_args), std::move(this->else_expression)};
            }

            simple_case_t<R, T, E, Args...> end() {
                return {std::move(this->case_expression), std::move(args), std::move(this->else_expression)};
            }

            template<class El>
            simple_case_builder<R, T, El, Args...> else_(El el) {
                return {{std::move(this->case_expression)}, std::move(args), {std::move(el)}};
            }
        };

        template<class T>
        constexpr void validate_conditions() {
            static_assert(count_tuple<T, is_where>::value <= 1, "a single query cannot contain > 1 WHERE blocks");
            static_assert(count_tuple<T, is_group_by>::value <= 1, "a single query cannot contain > 1 GROUP BY blocks");
            static_assert(count_tuple<T, is_order_by>::value <= 1, "a single query cannot contain > 1 ORDER BY blocks");
            static_assert(count_tuple<T, is_limit>::value <= 1, "a single query cannot contain > 1 LIMIT blocks");
            static_assert(count_tuple<T, is_from>::value <= 1, "a single query cannot contain > 1 FROM blocks");
        }
    }

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    internal::as_optional_t<T> as_optional(T value) {
        return {std::move(value)};
    }
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class T>
    internal::then_t<T> then(T t) {
        return {std::move(t)};
    }

    template<class R, class T>
    internal::simple_case_builder<R, T, void> case_(T t) {
        return {{std::move(t)}};
    }

    template<class R>
    internal::simple_case_builder<R, void, void> case_() {
        return {};
    }

    template<class T>
    internal::distinct_t<T> distinct(T t) {
        return {std::move(t)};
    }

    template<class T>
    internal::all_t<T> all(T t) {
        return {std::move(t)};
    }

    template<class... Args>
    internal::columns_t<Args...> distinct(internal::columns_t<Args...> cols) {
        cols.distinct = true;
        return cols;
    }

    /*
     *  Combine multiple columns in a tuple.
     */
    template<class... Args>
    constexpr internal::columns_t<Args...> columns(Args... args) {
        return {std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }

    /*
     *  Construct an unmapped structure ad-hoc from multiple columns.
     *  `T` must be constructible from the column results using direct-list-initialization.
     */
    template<class T, class... Args>
    constexpr internal::struct_t<T, Args...> struct_(Args... args) {
        return {std::make_tuple<Args...>(std::forward<Args>(args)...)};
    }

    /**
     *  Public function for subselect query. Is useful in UNION queries.
     */
    template<class T, class... Args>
    constexpr internal::select_t<T, Args...> select(T t, Args... args) {
        using args_tuple = std::tuple<Args...>;
        internal::validate_conditions<args_tuple>();
        return {std::move(t), {std::forward<Args>(args)...}};
    }

    /**
     *  Public function for UNION operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class... E>
    constexpr internal::union_t<E...> union_(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        return {{std::forward<E>(expressions)...}, false};
    }

    /**
     *  Public function for UNION ALL operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/union.cpp
     */
    template<class... E>
    constexpr internal::union_t<E...> union_all(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        return {{std::forward<E>(expressions)...}, true};
    }

    /**
     *  Public function for EXCEPT operator.
     *  Expressions are subselect objects.
     *  Look through example in examples/except.cpp
     */
    template<class... E>
    constexpr internal::except_t<E...> except(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        return {{std::forward<E>(expressions)...}};
    }

    template<class... E>
    constexpr internal::intersect_t<E...> intersect(E... expressions) {
        static_assert(sizeof...(E) >= 2, "Compound operators must have at least 2 select statements");
        return {{std::forward<E>(expressions)...}};
    }

#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#if SQLITE_VERSION_NUMBER >= 3035003
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /*
     *  Materialization hint to instruct SQLite to materialize the select statement of a CTE into an ephemeral table as an "optimization fence".
     *  
     *  Example:
     *  1_ctealias().as<materialized()>(select(1));
     */
    inline consteval internal::materialized_t materialized() {
        return {};
    }

    /*
     *  Materialization hint to instruct SQLite to substitute a CTE's select statement as a subquery subject to optimization.
     *  
     *  Example:
     *  1_ctealias().as<not_materialized()>(select(1));
     */
    inline consteval internal::not_materialized_t not_materialized() {
        return {};
    }
#endif
#endif

    /**
     *  Introduce the construction of a common table expression using the specified moniker.
     *  
     *  The list of explicit columns is optional;
     *  if provided the number of columns must match the number of columns of the subselect.
     *  The column names will be merged with the subselect:
     *  1. column names of subselect
     *  2. explicit columns
     *  3. fill in empty column names with column index
     *  
     *  Example:
     *  using cte_1 = decltype(1_ctealias);
     *  cte<cte_1>()(select(&Object::id));
     *  cte<cte_1>(&Object::name)(select("object"));
     */
    template<class Moniker,
             class... ExplicitCols,
             std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<
                                  internal::is_column_alias<ExplicitCols>,
                                  std::is_member_pointer<ExplicitCols>,
                                  internal::is_column<ExplicitCols>,
                                  std::is_same<ExplicitCols, polyfill::remove_cvref_t<decltype(std::ignore)>>,
                                  std::is_convertible<ExplicitCols, std::string>>...>,
                              bool> = true>
    constexpr auto cte(ExplicitCols... explicitColumns) {
        using namespace ::sqlite_orm::internal;
        static_assert(is_cte_moniker_v<Moniker>, "Moniker must be a CTE moniker");
        static_assert((!is_builtin_numeric_column_alias_v<ExplicitCols> && ...),
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE.");

        using builder_type =
            cte_builder<Moniker, transform_tuple_t<std::tuple<ExplicitCols...>, decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_cte_moniker auto moniker, class... ExplicitCols>
        requires((internal::is_column_alias_v<ExplicitCols> || std::is_member_pointer_v<ExplicitCols> ||
                  internal::is_column_v<ExplicitCols> ||
                  std::same_as<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>> ||
                  std::convertible_to<ExplicitCols, std::string>) &&
                 ...)
    constexpr auto cte(ExplicitCols... explicitColumns) {
        using namespace ::sqlite_orm::internal;
        static_assert((!is_builtin_numeric_column_alias_v<ExplicitCols> && ...),
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE.");

        using builder_type =
            cte_builder<decltype(moniker), transform_tuple_t<std::tuple<ExplicitCols...>, decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }
#endif

    namespace internal {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        template<char A, char... X>
        template<class... ExplicitCols>
            requires((is_column_alias_v<ExplicitCols> || std::is_member_pointer_v<ExplicitCols> ||
                      std::same_as<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>> ||
                      std::convertible_to<ExplicitCols, std::string>) &&
                     ...)
        constexpr auto cte_moniker<A, X...>::operator()(ExplicitCols... explicitColumns) const {
            return cte<cte_moniker<A, X...>>(std::forward<ExplicitCols>(explicitColumns)...);
        }
#else
        template<char A, char... X>
        template<class... ExplicitCols,
                 std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<
                                      is_column_alias<ExplicitCols>,
                                      std::is_member_pointer<ExplicitCols>,
                                      std::is_same<ExplicitCols, polyfill::remove_cvref_t<decltype(std::ignore)>>,
                                      std::is_convertible<ExplicitCols, std::string>>...>,
                                  bool>>
        constexpr auto cte_moniker<A, X...>::operator()(ExplicitCols... explicitColumns) const {
            return cte<cte_moniker<A, X...>>(std::forward<ExplicitCols>(explicitColumns)...);
        }
#endif
    }

    /** 
     *  With-clause for a tuple of ordinary CTEs.
     *  
     *  Despite the missing RECURSIVE keyword, the CTEs can be recursive.
     */
    template<class E, class... CTEs, internal::satisfies_not<internal::is_compound_operator, E> = true>
    internal::with_t<E, CTEs...> with(internal::common_table_expressions<CTEs...> ctes, E expression) {
        return {false, std::move(ctes), std::move(expression)};
    }

    /** 
     *  With-clause for a tuple of ordinary CTEs.
     *  
     *  Despite the missing RECURSIVE keyword, the CTEs can be recursive.
     */
    template<class Compound, class... CTEs, internal::satisfies<internal::is_compound_operator, Compound> = true>
    internal::with_t<internal::select_t<Compound>, CTEs...> with(internal::common_table_expressions<CTEs...> ctes,
                                                                 Compound sel) {
        return {false, std::move(ctes), sqlite_orm::select(std::move(sel))};
    }

    /** 
     *  With-clause for a single ordinary CTE.
     *  
     *  Despite the missing `RECURSIVE` keyword, the CTE can be recursive.
     *  
     *  Example:
     *  constexpr orm_cte_moniker auto cte_1 = 1_ctealias;
     *  with(cte_1().as(select(&Object::id)), select(cte_1->*1_colalias));
     */
    template<class E,
             class CTE,
             internal::satisfies_is_specialization_of<CTE, internal::common_table_expression> = true,
             internal::satisfies_not<internal::is_compound_operator, E> = true>
    internal::with_t<E, CTE> with(CTE cte, E expression) {
        return {false, {std::move(cte)}, std::move(expression)};
    }

    /** 
     *  With-clause for a single ordinary CTE.
     *  
     *  Despite the missing `RECURSIVE` keyword, the CTE can be recursive.
     *  
     *  Example:
     *  constexpr orm_cte_moniker auto cte_1 = 1_ctealias;
     *  with(cte_1().as(select(&Object::id)), select(cte_1->*1_colalias));
     */
    template<class Compound,
             class CTE,
             internal::satisfies_is_specialization_of<CTE, internal::common_table_expression> = true,
             internal::satisfies<internal::is_compound_operator, Compound> = true>
    internal::with_t<internal::select_t<Compound>, CTE> with(CTE cte, Compound sel) {
        return {false, {std::move(cte)}, sqlite_orm::select(std::move(sel))};
    }

    /** 
     *  With-clause for a tuple of potentially recursive CTEs.
     *  
     *  @note The use of RECURSIVE does not force common table expressions to be recursive.
     */
    template<class E, class... CTEs, internal::satisfies_not<internal::is_compound_operator, E> = true>
    internal::with_t<E, CTEs...> with_recursive(internal::common_table_expressions<CTEs...> ctes, E expression) {
        return {true, std::move(ctes), std::move(expression)};
    }

    /** 
     *  With-clause for a tuple of potentially recursive CTEs.
     *  
     *  @note The use of RECURSIVE does not force common table expressions to be recursive.
     */
    template<class Compound, class... CTEs, internal::satisfies<internal::is_compound_operator, Compound> = true>
    internal::with_t<internal::select_t<Compound>, CTEs...>
    with_recursive(internal::common_table_expressions<CTEs...> ctes, Compound sel) {
        return {true, std::move(ctes), sqlite_orm::select(std::move(sel))};
    }

    /** 
     *  With-clause for a single potentially recursive CTE.
     *  
     *  @note The use of RECURSIVE does not force common table expressions to be recursive.
     *  
     *  Example:
     *  constexpr orm_cte_moniker auto cte_1 = 1_ctealias;
     *  with_recursive(cte_1().as(select(&Object::id)), select(cte_1->*1_colalias));
     */
    template<class E,
             class CTE,
             internal::satisfies_is_specialization_of<CTE, internal::common_table_expression> = true,
             internal::satisfies_not<internal::is_compound_operator, E> = true>
    internal::with_t<E, CTE> with_recursive(CTE cte, E expression) {
        return {true, {std::move(cte)}, std::move(expression)};
    }

    /** 
     *  With-clause for a single potentially recursive CTE.
     *  
     *  @note The use of RECURSIVE does not force common table expressions to be recursive.
     *  
     *  Example:
     *  constexpr orm_cte_moniker auto cte_1 = 1_ctealias;
     *  with_recursive(cte_1().as(select(&Object::id)), select(cte_1->*1_colalias));
     */
    template<class Compound,
             class CTE,
             internal::satisfies_is_specialization_of<CTE, internal::common_table_expression> = true,
             internal::satisfies<internal::is_compound_operator, Compound> = true>
    internal::with_t<internal::select_t<Compound>, CTE> with_recursive(CTE cte, Compound sel) {
        return {true, {std::move(cte)}, sqlite_orm::select(std::move(sel))};
    }
#endif

    /**
     *   `SELECT * FROM T` expression that fetches results as tuples.
     *   T is a type mapped to a storage, or an alias of it.
     *   The `definedOrder` parameter denotes the expected order of result columns.
     *   The default is the implicit order as returned by SQLite, which may differ from the defined order
     *   if the schema of a table has been changed.
     *   By specifying the defined order, the columns are written out in the resulting select SQL string.
     *
     *   In pseudo code:
     *   select(asterisk<User>(false)) -> SELECT * from User
     *   select(asterisk<User>(true))  -> SELECT id, name from User
     *
     *   Example: auto rows = storage.select(asterisk<User>());
     *   // decltype(rows) is std::vector<std::tuple<...all columns in implicitly stored order...>>
     *   Example: auto rows = storage.select(asterisk<User>(true));
     *   // decltype(rows) is std::vector<std::tuple<...all columns in declared make_table order...>>
     *   
     *   If you need to fetch results as objects instead of tuples please use `object<T>()`.
     */
    template<class T>
    constexpr internal::asterisk_t<T> asterisk(bool definedOrder = false) {
        return {definedOrder};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Example:
     *  constexpr orm_table_alias auto m = "m"_alias.for_<Employee>();
     *  auto reportingTo = 
     *      storage.select(asterisk<m>(), inner_join<m>(on(m->*&Employee::reportsTo == &Employee::employeeId)));
     */
    template<orm_refers_to_recordset auto recordset>
    constexpr auto asterisk(bool definedOrder = false) {
        return asterisk<internal::auto_decay_table_ref_t<recordset>>(definedOrder);
    }
#endif

    /**
     *   `SELECT * FROM T` expression that fetches results as objects of type T.
     *   T is a type mapped to a storage, or an alias of it.
     *   
     *   Example: auto rows = storage.select(object<User>());
     *   // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in implicitly stored order
     *   Example: auto rows = storage.select(object<User>(true));
     *   // decltype(rows) is std::vector<User>, where the User objects are constructed from columns in declared make_table order
     *
     *   If you need to fetch results as tuples instead of objects please use `asterisk<T>()`.
     */
    template<class T>
    constexpr internal::object_t<T> object(bool definedOrder = false) {
        return {definedOrder};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_refers_to_table auto als>
    constexpr auto object(bool definedOrder = false) {
        return object<internal::auto_decay_table_ref_t<als>>(definedOrder);
    }
#endif
}
