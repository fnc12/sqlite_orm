#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3009000
#include <tuple>  //  std::make_tuple
#include <utility>  //  std::forward
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"
#include "../constraints.h"

#if SQLITE_VERSION_NUMBER >= 3009000
namespace sqlite_orm::internal {
    template<class T>
    using is_fts5_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                                                               check_if_is_template<prefix_t>,
                                                                               check_if_is_template<tokenize_t>,
                                                                               check_if_is_template<content_t>,
                                                                               check_if_is_template<table_content_t>>,
                                                              T>;
    struct fts5_module_tag {
        // simplify conceptual/meta programming
        using module_type = fts5_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "fts5";
        }
    };
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a FTS5 virtual table definition.
     */
    template<class... Cs>
    internal::virtual_table_definition<internal::fts5_module_tag, Cs...> using_fts5(Cs... definition) {
        static_assert(polyfill::conjunction_v<internal::is_fts5_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(definition)...)});
    }

    /**
     *  Factory function for a FTS5 virtual table definition.
     *  
     *  The mapped object type is explicitly specified.
     *  
     *  [Deprecation notice] This factory function is deprecated and will be removed in v1.11.
     */
    template<class T, class... Cs>
    [[deprecated("Specify the explicit object type when calling `make_virtual_table()`.")]]
    internal::virtual_table_description<T, internal::fts5_module_tag, Cs...> using_fts5(Cs... definition) {
        static_assert(polyfill::conjunction_v<internal::is_fts5_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(definition)...)});
    }
}
#endif
