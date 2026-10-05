#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <type_traits>  //  std::remove_const
#include <tuple>
#include <string>
#include <vector>
#endif
#endif

#include "tuple_helper/tuple_fy.h"
#include "column_result.h"
#include "vocabulary/node_traits.h"
#include "schema/column.h"
#include "schema/table_base.h"
#include "ast/quoted_expression.h"
#include "ast/result_columns.h"
#include "ast/select.h"
#include "ast/cte.h"
#include "alias.h"
#include "cte_types.h"
#include "serialization/cte_column_names_collector.h"
#include "column_expression.h"
#include "schema/db_objects.h"
#include "schema/algorithms/table_lookup.h"

namespace sqlite_orm::internal {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    // F = field_type
    template<typename Moniker,
             typename ExplicitColRefs,
             typename Expression,
             typename SubselectColRefs,
             typename FinalColRefs,
             typename F>
    struct create_cte_mapper {
        using type = subselect_mapper<Moniker, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, F>;
    };

    // std::tuple<Fs...>
    template<typename Moniker,
             typename ExplicitColRefs,
             typename Expression,
             typename SubselectColRefs,
             typename FinalColRefs,
             typename... Fs>
    struct create_cte_mapper<Moniker, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, std::tuple<Fs...>> {
        using type = subselect_mapper<Moniker, ExplicitColRefs, Expression, SubselectColRefs, FinalColRefs, Fs...>;
    };

    template<typename Moniker,
             typename ExplicitColRefs,
             typename Expression,
             typename SubselectColRefs,
             typename FinalColRefs,
             typename Result>
    using create_cte_mapper_t = typename create_cte_mapper<std::remove_const_t<Moniker>,
                                                           ExplicitColRefs,
                                                           Expression,
                                                           SubselectColRefs,
                                                           FinalColRefs,
                                                           Result>::type;

    template<class Mapper, class... Cs>
    struct cte_table : table_identifier, table_definition<Cs...> {
        using definition_type = table_definition<Cs...>;
        using cte_mapper_type = Mapper;
        using cte_moniker_type = typename cte_mapper_type::cte_moniker_type;
        using object_type = cte_moniker_type;
        using elements_type = typename definition_type::elements_type;
    };

    template<class Mapper, class... Cs>
    cte_table<Mapper, Cs...> make_cte_table(std::string name, Cs... args) {
        return {std::move(name), std::make_tuple<Cs...>(std::forward<Cs>(args)...)};
    }

    // aliased column expressions, explicit or implicitly numbered
    template<typename F, typename ColRef, satisfies_is_specialization_of<ColRef, alias_holder> = true>
    auto make_cte_column(std::string name, const ColRef& /*finalColRef*/) {
        using object_type = aliased_field<type_t<ColRef>, F>;

        return sqlite_orm::make_column<>(std::move(name), &object_type::field);
    }

    // F O::*
    template<typename F, typename ColRef, satisfies<std::is_member_pointer, ColRef> = true>
    auto make_cte_column(std::string name, const ColRef& finalColRef) {
        using column_type = column_t<ColRef, empty_setter>;

        return column_type{std::move(name), finalColRef, empty_setter{}, std::tuple<>{}};
    }

#ifdef SQLITE_ORM_STRUCTURED_BINDING_PACK_SUPPORTED
    /**
     *  Concatenate newly created tables with given DBOs, forming a new set of DBOs.
     */
    template<typename DBOs, typename... CTETables>
    auto db_objects_cat(const DBOs& dbObjects, CTETables&&... cteTables) {
        auto& [... elements] = dbObjects;
        return std::tuple{std::forward<CTETables>(cteTables)..., elements...};
    }
#else
    /**
     *  Concatenate newly created tables with given DBOs, forming a new set of DBOs.
     */
    template<typename DBOs, size_t... Idx, typename... CTETables>
    auto db_objects_cat(const DBOs& dbObjects, std::index_sequence<Idx...>, CTETables&&... cteTables) {
        return std::tuple{std::forward<CTETables>(cteTables)..., std::get<Idx>(dbObjects)...};
    }

    /**
     *  Concatenate newly created tables with given DBOs, forming a new set of DBOs.
     */
    template<typename DBOs, typename... CTETables>
    auto db_objects_cat(const DBOs& dbObjects, CTETables&&... cteTables) {
        return db_objects_cat(dbObjects,
                              std::make_index_sequence<std::tuple_size_v<DBOs>>{},
                              std::forward<CTETables>(cteTables)...);
    }
#endif

    /**
     *  This function returns the expression contained in a subselect that is relevant for
     *  creating the definition of a CTE table.
     *  Because CTEs can recursively refer to themselves in a compound statement, parsing
     *  the whole compound statement would lead to compiler errors if a column_pointer<>
     *  can't be resolved. Therefore, at the time of building a CTE table, we are only
     *  interested in the column results of the left-most select expression.
     */
    template<class Select, satisfies<is_select, Select> = true>
    decltype(auto) get_cte_driving_subselect(const Select& subSelect) {
        if constexpr (is_compound_operator_v<return_type_t<Select>>) {
            // left-most select expression of compound statement
            return std::get<0>(subSelect.col.compound);
        } else {
            return subSelect;
        }
    }

    /**
     *  Return a tuple of member pointers of all columns
     */
    template<class C, size_t... Idx>
    auto get_table_columns_fields(const C& coldef, std::index_sequence<Idx...>) {
        return std::make_tuple(get<Idx>(coldef).member_pointer...);
    }

    /**
     *  Return the column reference of a single result column expression, as a 1-tuple.
     *  `Idx` is the position of the result column, by which an expression without a name of its own is referenced.
     */
    template<size_t Idx, class DBOs, class E>
    auto extract_colref_expression(const DBOs& dbObjects, const E& col) {
        if constexpr (is_quoted_expression_v<E>) {
            return extract_colref_expression<Idx>(dbObjects, col._value);
        } else if constexpr (is_column_pointer_v<E>) {
            return extract_colref_expression<Idx>(dbObjects, col.field);
        } else if constexpr (std::is_member_pointer_v<E>) {
            // field/getter -> field/getter
            return std::make_tuple(col);
        } else if constexpr (is_as_node_v<E>) {
            // aliased expression -> alias_holder
            return std::tuple<alias_holder<alias_type_t<E>>>{};
        } else if constexpr (polyfill::is_specialization_of_v<E, alias_holder>) {
            // colref -> alias_holder
            return std::tuple<E>{};
        } else {
            // any expression -> numeric column alias
            return std::tuple<alias_holder<decltype(n_to_colalias<Idx>())>>{};
        }
    }

    template<class DBOs, class Tpl, size_t... Idx>
    auto extract_colref_expressions(const DBOs& dbObjects, const Tpl& cols, std::index_sequence<Idx...>) {
        return std::tuple_cat(extract_colref_expression<Idx>(dbObjects, std::get<Idx>(cols))...);
    }

    /**
     *  Return a tuple of the column references of a select's result columns.
     */
    template<class DBOs, class E>
    auto extract_colref_expressions(const DBOs& dbObjects, const E& col) {
        static_assert(!is_select_v<E> && !is_compound_operator_v<E>,
                      "A select statement cannot be a result column of a CTE's select statement");

        if constexpr (is_columns_v<E>) {
            return extract_colref_expressions(dbObjects,
                                              col.columns,
                                              std::make_index_sequence<std::tuple_size_v<columns_type_t<E>>>{});
        } else if constexpr (is_asterisk_v<E>) {
            // -> fields
            using O = type_t<E>;
            using table_type = schema_pick_table_t<O, DBOs>;
            using elements_type = elements_type_t<table_type>;
            using column_idxs = filter_tuple_sequence_t<elements_type, is_column>;

            auto& table = pick_table<O>(dbObjects);
            return get_table_columns_fields(table.elements, column_idxs{});
        } else {
            return extract_colref_expression<0>(dbObjects, col);
        }
    }

    /*
     *  Depending on ExplicitColRef's type returns either the explicit column reference
     *  or the expression's column reference otherwise.
     */
    template<typename DBOs, typename SubselectColRef, typename ExplicitColRef>
    auto determine_cte_colref(const DBOs& /*dbObjects*/,
                              const SubselectColRef& subselectColRef,
                              const ExplicitColRef& explicitColRef) {
        if constexpr (polyfill::is_specialization_of_v<ExplicitColRef, alias_holder>) {
            return explicitColRef;
        } else if constexpr (std::is_member_pointer<ExplicitColRef>::value) {
            return explicitColRef;
        } else if constexpr (is_column<ExplicitColRef>::value) {
            return explicitColRef.member_pointer;
        } else if constexpr (std::is_same_v<ExplicitColRef, std::string>) {
            return subselectColRef;
        } else if constexpr (std::is_same_v<ExplicitColRef, polyfill::remove_cvref_t<decltype(std::ignore)>>) {
            return subselectColRef;
        } else {
            static_assert(polyfill::always_false_v<ExplicitColRef>, "Invalid explicit column reference specified");
        }
    }

    template<typename DBOs, typename SubselectColRefs, typename ExplicitColRefs, size_t... Idx>
    auto determine_cte_colrefs([[maybe_unused]] const DBOs& dbObjects,
                               const SubselectColRefs& subselectColRefs,
                               [[maybe_unused]] const ExplicitColRefs& explicitColRefs,
                               std::index_sequence<Idx...>) {
        if constexpr (std::tuple_size_v<ExplicitColRefs> != 0) {
            static_assert((!is_builtin_numeric_column_alias_v<
                               alias_holder_type_or_none_t<std::tuple_element_t<Idx, ExplicitColRefs>>> &&
                           ...),
                          "Numeric column aliases are reserved for referencing columns locally within a single CTE");

            return std::tuple{
                determine_cte_colref(dbObjects, std::get<Idx>(subselectColRefs), std::get<Idx>(explicitColRefs))...};
        } else {
            return subselectColRefs;
        }
    }

    template<typename Mapper, typename DBOs, typename ColRefs, size_t... CIs>
    auto make_cte_table_using_column_indices(const DBOs& /*dbObjects*/,
                                             std::string tableName,
                                             std::vector<std::string> columnNames,
                                             const ColRefs& finalColRefs,
                                             std::index_sequence<CIs...>) {
        return make_cte_table<Mapper>(
            std::move(tableName),
            make_cte_column<std::tuple_element_t<CIs, typename Mapper::fields_type>>(std::move(columnNames.at(CIs)),
                                                                                     std::get<CIs>(finalColRefs))...);
    }

    template<typename DBOs, typename CTE>
    auto make_cte_db_object(const DBOs& dbObjects, const CTE& cte) {
        using cte_type = CTE;

        auto subSelect = get_cte_driving_subselect(cte.subselect);

        using subselect_type = decltype(subSelect);
        using column_results = column_result_of_t<DBOs, subselect_type>;
        using index_sequence = std::make_index_sequence<std::tuple_size_v<tuplify_t<column_results>>>;
        static_assert(cte_type::explicit_colref_count == 0 || cte_type::explicit_colref_count == index_sequence::size(),
                      "Number of explicit columns of common table expression doesn't match the number of columns "
                      "in the subselect.");

        std::string tableName = alias_extractor<cte_moniker_type_t<cte_type>>::extract();
        auto subselectColRefs = extract_colref_expressions(dbObjects, subSelect.col);
        const auto& finalColRefs =
            determine_cte_colrefs(dbObjects, subselectColRefs, cte.explicitColumns, index_sequence{});

        serializer_context context{dbObjects};
        std::vector<std::string> columnNames = resolve_cte_column_names(subSelect, cte.explicitColumns, context);

        using mapper_type = create_cte_mapper_t<cte_moniker_type_t<cte_type>,
                                                explicit_colrefs_tuple_t<cte_type>,
                                                column_expression_of_t<DBOs, subselect_type>,
                                                decltype(subselectColRefs),
                                                polyfill::remove_cvref_t<decltype(finalColRefs)>,
                                                column_results>;
        return make_cte_table_using_column_indices<mapper_type>(dbObjects,
                                                                std::move(tableName),
                                                                std::move(columnNames),
                                                                finalColRefs,
                                                                index_sequence{});
    }

    template<typename DBOs, typename... CTEs, size_t Ii, size_t... In>
    decltype(auto) make_recursive_cte_db_objects(const DBOs& dbObjects,
                                                 const common_table_expressions<CTEs...>& cte,
                                                 std::index_sequence<Ii, In...>) {
        auto tbl = make_cte_db_object(dbObjects, std::get<Ii>(cte));

        if constexpr (sizeof...(In) > 0) {
            return make_recursive_cte_db_objects(
                // Because CTEs can depend on their predecessor we recursively pass in a new set of DBOs
                db_objects_cat(dbObjects, std::move(tbl)),
                cte,
                std::index_sequence<In...>{});
        } else {
            return db_objects_cat(dbObjects, std::move(tbl));
        }
    }

    /**
     *  Return new DBOs for CTE expressions.
     */
    template<class DBOs,
             class With,
             std::enable_if_t<std::conjunction_v<is_db_objects<DBOs>, is_with_clause<With>>, bool> = true>
    decltype(auto) db_objects_for_expression(DBOs& dbObjects, const With& e) {
        return make_recursive_cte_db_objects(dbObjects,
                                             e.cte,
                                             std::make_index_sequence<std::tuple_size_v<cte_type_t<With>>>{});
    }
#endif
}
