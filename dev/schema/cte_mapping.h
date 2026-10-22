#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#include <type_traits>  //  std::is_member_pointer_v
#include <tuple>  //  std::tuple, std::make_tuple, std::get, std::tuple_element_t
#include <string>  //  std::string
#include <utility>  //  std::index_sequence, std::forward, std::move
#include <vector>  //  std::vector
#endif
#endif

#include "../functional/cxx_polyfill/type_traits.h"
#include "../functional/type_traits.h"
#include "../functional/tuple_helper/tuple_fy.h"
#include "../vocabulary/node_traits.h"
#include "column.h"
#include "table_base.h"
#include "table_identifier.h"

#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
namespace sqlite_orm::internal {
    /**
     *  Aliased column expression mapped into a CTE, stored as a field in a table column.
     */
    template<class A, class F>
    struct aliased_field {
        ~aliased_field() = delete;
        aliased_field(const aliased_field&) = delete;
        void operator=(const aliased_field&) = delete;

        F field;
    };

    /**
     *  This class captures various properties and aspects of a subselect's column expression,
     *  and is used as a proxy in base_table<>.
     *
     *  Named after CTEs, the only subselects mapped as a table so far.
     */
    template<typename Moniker,
             typename ExplicitColRefs,
             typename Expression,
             typename SubselectColRefs,
             typename FinalColRefs,
             typename ColumnResults>
    class cte_mapper {
      public:
        cte_mapper() = delete;

        // this type name is used to detect the mapping from moniker to object
        using cte_moniker_type = Moniker;
        using fields_type = tuplify_t<ColumnResults>;
        // this type captures the expressions forming the columns in a subselect;
        // it is currently unused, however proves to be useful in compilation errors,
        // as it simplifies recognizing errors in column expressions
        using expressions_tuple = tuplify_t<Expression>;
        // this type captures column reference expressions specified at CTE construction;
        // those are: member pointers, alias holders
        using explicit_colrefs_tuple = ExplicitColRefs;
        // this type captures column reference expressions from the subselect;
        // those are: member pointers, alias holders
        using subselect_colrefs_tuple = SubselectColRefs;
        // this type captures column reference expressions merged from SubselectColRefs and ExplicitColRefs
        using final_colrefs_tuple = FinalColRefs;
    };

    /**
     *  A CTE mapped as a table, its object type being the CTE moniker.
     */
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

    /**
     *  Make the column of a CTE table from its final column reference:
     *  - an aliased column expression, explicit or implicitly numbered, is stored as a field of `aliased_field<>`;
     *  - a member pointer is used as is.
     */
    template<typename F, typename ColRef>
    auto make_cte_column(std::string name, [[maybe_unused]] const ColRef& finalColRef) {
        if constexpr (is_alias_holder_v<ColRef>) {
            using object_type = aliased_field<type_t<ColRef>, F>;

            return sqlite_orm::make_column<>(std::move(name), &object_type::field);
        } else if constexpr (std::is_member_pointer_v<ColRef>) {
            using column_type = column_t<ColRef, empty_setter>;

            return column_type{std::move(name), finalColRef, empty_setter{}, std::tuple<>{}};
        } else {
            static_assert(polyfill::always_false_v<ColRef>, "Invalid final column reference");
        }
    }

    template<typename Mapper, typename ColRefs, size_t... CIs>
    auto make_cte_table_using_column_indices(std::string tableName,
                                             std::vector<std::string> columnNames,
                                             const ColRefs& finalColRefs,
                                             std::index_sequence<CIs...>) {
        return make_cte_table<Mapper>(
            std::move(tableName),
            make_cte_column<std::tuple_element_t<CIs, typename Mapper::fields_type>>(std::move(columnNames.at(CIs)),
                                                                                     std::get<CIs>(finalColRefs))...);
    }
}
#endif
