#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3009000
#include <tuple>  //  std::make_tuple
#include <utility>  //  std::forward, std::move
#endif
#endif

#include "../functional/cxx_optional.h"
#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../schema/virtual_table.h"
#include "../schema/column.h"
#include "../constraints.h"

#if SQLITE_VERSION_NUMBER >= 3009000
namespace sqlite_orm::internal {
    template<class T>
    inline constexpr bool is_fts5_table_element_or_constraint_v =
        mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                       check_if_is_template<prefix_t>,
                                       check_if_is_template<tokenize_t>,
                                       check_if_is_template<content_t>,
                                       check_if_is_template<table_content_t>>,
                      T>::value;
    struct fts5_module_tag {
        // simplify conceptual/meta programming
        using module_type = fts5_module_tag;

        static constexpr orm_gsl::czstring name() {
            return "fts5";
        }
    };

    template<class... Cs>
    inline virtual_table_definition<fts5_module_tag, Cs...> make_fts5_definition(Cs... definition) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {{std::make_tuple(std::move(definition)...)}});
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  Class namespace for hidden `fts5` columns.
     */
    struct fts5 {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        /** 
            Hidden columns of the `fts5` virtual table, which can be referred to using a 'column pointer'.
         */
        struct hidden {
            std::optional<int> rank;

            using enclosing_type = fts5;
        };
#endif

        fts5() = delete;
    };

    /**
     *  Factory function for a FTS5 virtual table definition.
     *  
     *  The hidden FTS5 column 'rank' is mapped into each table definition and can be used via an explicit column pointer.
     */
    template<class... Cs>
    auto using_fts5(Cs... definition) {
        using namespace ::sqlite_orm::internal;
        return make_fts5_definition(std::forward<Cs>(definition)...
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                    ,
                                    make_hidden_column("rank", &fts5::hidden::rank)
#endif
        );
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
        static_assert((internal::is_fts5_table_element_or_constraint_v<Cs> && ...),
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(definition)...)});
    }
}
#endif
