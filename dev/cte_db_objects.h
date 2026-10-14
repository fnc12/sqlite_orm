#pragma once

/** @file The database objects of a WITH clause: its CTEs mapped as tables, prepended to the schema's database objects.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <type_traits>  //  std::is_member_pointer_v, std::is_same_v
#include <tuple>  //  std::tuple, std::get, std::ignore, std::tuple_size_v, std::tuple_element_t
#include <string>  //  std::string
#include <vector>  //  std::vector
#include <utility>  //  std::index_sequence, std::make_index_sequence, std::move
#endif
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_fy.h"
#include "vocabulary/node_traits.h"
#include "vocabulary/node_algorithms.h"  //  access_leftmost_select
#include "ast/cte.h"
#include "ast/alias.h"
#include "schema/cte_mapping.h"
#include "schema/db_objects.h"
#include "result_set/column_result.h"
#include "result_set/column_expression.h"
#include "result_set/column_references.h"
#include "serialization/serializer_context.h"
#include "serialization/cte_column_names_collector.h"

namespace sqlite_orm::internal {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    /*
     *  Depending on ExplicitColRef's type returns either the explicit column reference
     *  or the expression's column reference otherwise.
     */
    template<typename SubselectColRef, typename ExplicitColRef>
    auto resolve_cte_colref(const SubselectColRef& subselectColRef, const ExplicitColRef& explicitColRef) {
        if constexpr (is_alias_holder_v<ExplicitColRef>) {
            return explicitColRef;
        } else if constexpr (std::is_member_pointer_v<ExplicitColRef>) {
            return explicitColRef;
        } else if constexpr (is_column_v<ExplicitColRef>) {
            return explicitColRef.member_pointer;
        } else if constexpr (std::is_same_v<ExplicitColRef, std::string>) {
            return subselectColRef;
        } else if constexpr (std::is_same_v<ExplicitColRef, polyfill::remove_cvref_t<decltype(std::ignore)>>) {
            return subselectColRef;
        } else {
            static_assert(polyfill::always_false_v<ExplicitColRef>, "Invalid explicit column reference specified");
        }
    }

    template<typename SubselectColRefs, typename ExplicitColRefs, size_t... Idx>
    auto resolve_cte_colrefs(const SubselectColRefs& subselectColRefs,
                             [[maybe_unused]] const ExplicitColRefs& explicitColRefs,
                             std::index_sequence<Idx...>) {
        if constexpr (std::tuple_size_v<ExplicitColRefs> != 0) {
            static_assert((!is_builtin_numeric_column_alias_v<
                               alias_holder_type_or_none_t<std::tuple_element_t<Idx, ExplicitColRefs>>> &&
                           ...),
                          "Numeric column aliases are reserved for referencing columns locally within a single CTE");

            return std::tuple{resolve_cte_colref(std::get<Idx>(subselectColRefs), std::get<Idx>(explicitColRefs))...};
        } else {
            return subselectColRefs;
        }
    }

    template<typename DBOs, typename CTE>
    auto make_cte_db_object(const DBOs& dbObjects, const CTE& cte) {
        using cte_type = CTE;

        auto subSelect = access_leftmost_select(cte.subselect);

        using subselect_type = decltype(subSelect);
        using column_results = column_result_of_t<DBOs, subselect_type>;
        using index_sequence = std::make_index_sequence<std::tuple_size_v<tuplify_t<column_results>>>;
        static_assert(cte_type::explicit_colref_count == 0 || cte_type::explicit_colref_count == index_sequence::size(),
                      "Number of explicit columns of common table expression doesn't match the number of columns "
                      "in the subselect.");

        std::string tableName = alias_extractor<cte_moniker_type_t<cte_type>>::extract();
        auto subselectColRefs = get_column_references_of(dbObjects, subSelect);
        const auto& finalColRefs = resolve_cte_colrefs(subselectColRefs, cte.explicitColumns, index_sequence{});

        serializer_context context{dbObjects};
        std::vector<std::string> columnNames = resolve_cte_column_names(subSelect, cte.explicitColumns, context);

        using mapper_type = cte_mapper<cte_moniker_type_t<cte_type>,
                                       explicit_colrefs_tuple_t<cte_type>,
                                       column_expression_of_t<DBOs, subselect_type>,
                                       decltype(subselectColRefs),
                                       polyfill::remove_cvref_t<decltype(finalColRefs)>,
                                       column_results>;
        return make_cte_table_using_column_indices<mapper_type>(std::move(tableName),
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
