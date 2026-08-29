#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>
#endif
#include <type_traits>  //  std::enable_if, std::remove_cvref, std::remove_const, std::is_convertible, std::is_same, std::is_member_pointer
#include <string>  //  std::string
#include <string_view>  //  std::string_view
#include <utility>  //  std::forward, std::move
#include <tuple>  //  std::tuple, std::ignore, std::tuple_size
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tuple_helper/tuple_transformer.h"
#include "../type_traits.h"
#include "../alias.h"
#include "../alias_traits.h"
#include "../cte_moniker.h"
#include "select.h"
#include "compound_operator.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "../vocabulary/traits/semantic_traits_fwd.h"  // Included to specialize traits

namespace sqlite_orm::internal {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
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
    struct materialized_t {
        constexpr operator std::string_view() const {
            return "MATERIALIZED";
        }
    };

    /*
     *  Materialization hint to instruct SQLite to substitute a CTE's select statement as a subquery subject to optimization.
     */
    struct not_materialized_t {
        constexpr operator std::string_view() const {
            return "NOT MATERIALIZED";
        }
    };

    template<class T>
    constexpr bool is_materialization_hint_v =
        std::disjunction<std::is_same<T, materialized_t>, std::is_same<T, not_materialized_t>>::value;
#else
    template<class T>
    constexpr bool is_materialization_hint_v = false;
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
        static constexpr size_t explicit_colref_count = std::tuple_size<ExplicitCols>::value;

        SQLITE_ORM_NOUNIQUEADDRESS hints_tuple hints;
        explicit_colrefs_tuple explicitColumns;
        expression_type subselect;

        constexpr common_table_expression(explicit_colrefs_tuple explicitColumns, expression_type subselect) :
            explicitColumns{std::move(explicitColumns)}, subselect{std::move(subselect)} {
            this->subselect.highest_level = true;
        }
    };

    template<class T>
    constexpr bool is_cte_binding_v = polyfill::is_specialization_of<T, common_table_expression>::value;

    template<class... CTEs>
    using common_table_expressions = std::tuple<CTEs...>;

    template<typename Moniker, class ExplicitCols>
    struct cte_builder {
        ExplicitCols _explicitColumns;

#if SQLITE_VERSION_NUMBER >= 3035000 && defined(SQLITE_ORM_WITH_CPP20_ALIASES)
        template<auto... hints, class Select, satisfies<is_select, Select> = true>
        constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<decltype(hints)...>, Select>
        as(Select sel) && {
            return {std::move(_explicitColumns), std::move(sel)};
        }

        template<auto... hints, class Compound, satisfies<is_compound_operator, Compound> = true>
        constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<decltype(hints)...>, select_t<Compound>>
        as(Compound sel) && {
            return {std::move(_explicitColumns), {std::move(sel)}};
        }
#else
        template<class Select, satisfies<is_select, Select> = true>
        constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<>, Select> as(Select sel) && {
            return {std::move(_explicitColumns), std::move(sel)};
        }

        template<class Compound, satisfies<is_compound_operator, Compound> = true>
        constexpr common_table_expression<Moniker, ExplicitCols, std::tuple<>, select_t<Compound>> as(Compound sel) && {
            return {std::move(_explicitColumns), {std::move(sel)}};
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
            if constexpr (is_select_v<expression_type>) {
                this->expression.highest_level = true;
            }
        }
    };

    template<class T>
    constexpr bool is_with_clause_v = polyfill::is_specialization_of<T, with_t>::value;
#else
    template<class T>
    constexpr bool is_cte_binding_v = false;

    template<class T>
    constexpr bool is_materialization_hint_v = false;

    template<class T>
    constexpr bool is_with_clause_v = false;
#endif

    template<class With>
    constexpr bool is_select_expression_v<
        With,
        std::enable_if_t<std::conjunction_v<is_with_clause<With>, is_select<expression_type_t<With>>>>> = true;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#if SQLITE_VERSION_NUMBER >= 3035003
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /*
     *  Materialization hint to instruct SQLite to materialize the select statement of a CTE into an ephemeral table as an "optimization fence".
     *
     *  Example:
     *  1_ctealias().as<materialized()>(select(1));
     */
    consteval internal::materialized_t materialized() {
        return {};
    }

    /*
     *  Materialization hint to instruct SQLite to substitute a CTE's select statement as a subquery subject to optimization.
     *
     *  Example:
     *  1_ctealias().as<not_materialized()>(select(1));
     */
    consteval internal::not_materialized_t not_materialized() {
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
             std::enable_if_t<std::conjunction_v<std::disjunction<
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
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE");

        using builder_type =
            cte_builder<Moniker, transform_tuple_t<std::tuple<ExplicitCols...>, decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<orm_cte_moniker auto moniker, class... ExplicitCols>
        requires ((internal::is_column_alias_v<ExplicitCols> || std::is_member_pointer_v<ExplicitCols> ||
                   internal::is_column_v<ExplicitCols> ||
                   std::same_as<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>> ||
                   std::convertible_to<ExplicitCols, std::string>) &&
                  ...)
    constexpr auto cte(ExplicitCols... explicitColumns) {
        using namespace ::sqlite_orm::internal;
        static_assert((!is_builtin_numeric_column_alias_v<ExplicitCols> && ...),
                      "Numeric column aliases are reserved for referencing columns locally within a single CTE");

        using builder_type = cte_builder<std::remove_const_t<decltype(moniker)>,
                                         transform_tuple_t<std::tuple<ExplicitCols...>, decay_explicit_column_t>>;
        return builder_type{{std::move(explicitColumns)...}};
    }
#endif

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
             internal::satisfies<internal::is_cte_binding, CTE> = true,
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
             internal::satisfies<internal::is_cte_binding, CTE> = true,
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
             internal::satisfies<internal::is_cte_binding, CTE> = true,
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
             internal::satisfies<internal::is_cte_binding, CTE> = true,
             internal::satisfies<internal::is_compound_operator, Compound> = true>
    internal::with_t<internal::select_t<Compound>, CTE> with_recursive(CTE cte, Compound sel) {
        return {true, {std::move(cte)}, sqlite_orm::select(std::move(sel))};
    }
#endif
}

// Implementation of cte_moniker's call operator which depends on `cte<>()`
namespace sqlite_orm::internal {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<char A, char... X>
    template<class... ExplicitCols>
        requires ((is_column_alias_v<ExplicitCols> || std::is_member_pointer_v<ExplicitCols> ||
                   std::same_as<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>> ||
                   std::convertible_to<ExplicitCols, std::string>) &&
                  ...)
    constexpr auto cte_moniker<A, X...>::operator()(ExplicitCols... explicitColumns) SQLITE_ORM_OR_CONST_CALLOP {
        return cte<cte_moniker<A, X...>>(std::forward<ExplicitCols>(explicitColumns)...);
    }
#else
    template<char A, char... X>
    template<class... ExplicitCols,
             std::enable_if_t<std::conjunction_v<std::disjunction<
                                  is_column_alias<ExplicitCols>,
                                  std::is_member_pointer<ExplicitCols>,
                                  std::is_same<ExplicitCols, polyfill::remove_cvref_t<decltype(std::ignore)>>,
                                  std::is_convertible<ExplicitCols, std::string>>...>,
                              bool>>
    constexpr auto cte_moniker<A, X...>::operator()(ExplicitCols... explicitColumns) SQLITE_ORM_OR_CONST_CALLOP {
        return cte<cte_moniker<A, X...>>(std::forward<ExplicitCols>(explicitColumns)...);
    }
#endif
#endif
}
