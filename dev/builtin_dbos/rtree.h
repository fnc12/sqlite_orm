#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ENABLE_RTREE
#include <type_traits>  // std::is_same
#include <tuple>  // std::tuple_element, std::make_tuple
#include <utility>  // std::forward
#include <cstdint>  //  std::int32_t
#endif
#endif

#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../tuple_helper/tuple_filter.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/node_algorithms.h"
#include "../schema/virtual_table.h"

#ifdef SQLITE_ENABLE_RTREE
namespace sqlite_orm::internal {
    template<class T>
    constexpr bool is_rtree_table_element_or_constraint_v =
        mpl::invoke_t<mpl::disjunction<check_if<is_column>>, T>::value;

    struct rtree_module_tag {
        // simplify conceptual/meta programming
        using module_type = rtree_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "rtree";
        }
    };

    struct rtree_i32_module_tag {
        // simplify conceptual/meta programming
        using module_type = rtree_i32_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "rtree_i32";
        }
    };

    template<class ExpectedValueType, class... Cs>
    constexpr void validate_rtree_definition() {
        using elements_type = std::tuple<Cs...>;
        using rtree_col_index_sequence = col_index_sequence_excluding<elements_type, is_auxiliary>;
        constexpr size_t nRTreeColumns = rtree_col_index_sequence::size();
        constexpr size_t nRTreeColumnsOfExpectedType =
            count_filtered_tuple<elements_type,
                                 check_if_is_type<ExpectedValueType>::template fn,
                                 rtree_col_index_sequence,
                                 field_type_t>::value;

        static_assert((is_rtree_table_element_or_constraint_v<Cs> && ...), "Incorrect table elements or constraints");
        static_assert(nRTreeColumns >= 3 && nRTreeColumns <= 11 && nRTreeColumns % 2 == 1,
                      "An RTREE table must have between 1 and 5 dimensions consisting of min/max-value pair columns");
        static_assert(
            nRTreeColumnsOfExpectedType == nRTreeColumns - 1,
            R"(The min/max-value pair columns need to be 32-bit floating point values for RTREE virtual tables and 32-bit signed integers for RTREE_I32 virtual tables, as they are stored as such)");
        static_assert(std::is_same<field_type_t<std::tuple_element_t<0, elements_type>>, int64>::value,
                      "The type of the first column must be a 64-bit integer");
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a RTREE virtual table definition.
     */
    template<class... Cs>
    internal::virtual_table_definition<internal::rtree_module_tag, Cs...> using_rtree(Cs... definition) {
        internal::validate_rtree_definition<float, Cs...>();

        return {std::make_tuple(std::forward<Cs>(definition)...)};
    }
    /**
     *  Factory function for a RTREE_I32 virtual table definition.
     */
    template<class... Cs>
    internal::virtual_table_definition<internal::rtree_i32_module_tag, Cs...> using_rtree_i32(Cs... definition) {
        internal::validate_rtree_definition<std::int32_t, Cs...>();

        return {std::make_tuple(std::forward<Cs>(definition)...)};
    }
}
#endif
