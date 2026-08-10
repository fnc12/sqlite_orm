#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
#include <tuple>  //  std::tuple_size, std::make_tuple, std::get
#include <utility>  //  std::forward, std::move
#include <vector>
#endif
#endif

#include "../functional/cxx_optional.h"
#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../member_traits/member_traits.h"
#include "../vocabulary/node_traits.h"
#include "../schema/constraints/prefix.h"
#include "../schema/constraints/tokenize.h"
#include "../schema/constraints/content.h"

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
namespace sqlite_orm::internal {
    template<class T>
    constexpr bool is_fts5_table_element_or_constraint_v =
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
        return {{std::make_tuple(std::move(definition)...)}};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /** 
     *  Class namespace for hidden `fts5` columns.
     */
    struct fts5 {
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        /** 
         *  Hidden columns of the `fts5` virtual table, which can be referred to using a 'column pointer' or using `hidden_fields_of<>` fields.
         */
        struct hidden {
            /// Hidden column with the same name as the table itself. It is a blob only meaningful when matching any of the other FTS5 columns.
            std::vector<char> any;
            std::optional<int> rank;

            /** 
             *  Internal meta programming helper to rebind hidden columns to a specific object type.
             */
            template<class O>
            struct _of;
        };

        /** 
         *  Map hidden columns to a specific object type to simplify programming.

         *  Example:
         *  struct Post {
         *   using hidden = fts5::hidden_fields_of<Post>;
         *  };
         */
        template<class O>
        struct hidden_fields_of {
            static constexpr auto any_field = internal::as_field_of<hidden::_of<O>>(&fts5::hidden::any);
            static constexpr auto rank_field = internal::as_field_of<hidden::_of<O>>(&fts5::hidden::rank);

            hidden_fields_of() = delete;
        };
#endif

        fts5() = delete;
    };

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
    template<class O>
    struct fts5::hidden::_of : fts5::hidden {
        using enclosing_type = O;
    };
#endif

    /**
     *  Factory function for a FTS5 virtual table definition.
     *  
     *  The hidden FTS5 columns 'any' [as a placeholder for the column with the same name as the table] and 'rank' are mapped into each table definition.
     */
    template<class... Cs>
    auto using_fts5(Cs... definition) {
        using namespace ::sqlite_orm::internal;
        return make_fts5_definition(std::forward<Cs>(definition)...
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
                                    ,
                                    // note: eponymous column name is filled in later in `make_virtual_table()`
                                    make_hidden_column("", &fts5::hidden::any),
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

        return {std::make_tuple(std::forward<Cs>(definition)...)};
    }

    template<class O, class... Cs>
    internal::virtual_table<O, internal::fts5_module_tag, Cs...>
    make_virtual_table(std::string tableName,
                       internal::virtual_table_definition<internal::fts5_module_tag, Cs...> definition) {
        using namespace ::sqlite_orm::internal;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
        constexpr size_t eponymous_column_index =
            std::tuple_size<elements_type_t<virtual_table_traits<fts5_module_tag, Cs...>>>::value - 2;
        // fill in the eponymous hidden column name
        std::get<eponymous_column_index>(definition.elements).name = tableName;
#endif

        return {std::move(tableName), std::move(definition)};
    }
}
#endif
