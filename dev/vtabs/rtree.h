#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  // std::is_same
#include <tuple>  // std::tuple_element, std::make_tuple
#include <utility>  // std::forward
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"

namespace sqlite_orm::internal {
    template<class T>
    using is_rtree_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>>, T>;

    struct rtree_module_tag {
        // simplify conceptual/meta programming
        using module_type = rtree_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "rtree";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for the RTREE virtual table definition.
     */
    template<class... Cs>
    internal::virtual_table_definition<internal::rtree_module_tag, Cs...> using_rtree(Cs... definition) {
        static_assert(polyfill::conjunction_v<internal::is_rtree_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");
        static_assert(sizeof...(Cs) >= 3 && sizeof...(Cs) <= 11 && sizeof...(Cs) % 2 == 1,
                      "An RTREE table must consist of at least 1 up to 5 dimensions");
        static_assert(std::is_same<typename std::tuple_element_t<0, std::tuple<Cs...>>::field_type, int64>::value,
                      "The type of the first column must be a 64-bit integer");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(definition)...)});
    }
}
