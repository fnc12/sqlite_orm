#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  // std::convertible_to
#endif
#include <string>  //  std::string
#include <tuple>  //  std::tuple_element, std::make_tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"
#include "../type_traits.h"
#include "../constraints.h"
#include "table_base.h"
#include "column.h"

namespace sqlite_orm::internal {

#if SQLITE_VERSION_NUMBER >= 3009000
    template<class T>
    using is_fts5_table_element_or_constraint = mpl::invoke_t<mpl::disjunction<check_if<is_column>,
                                                                               check_if_is_template<prefix_t>,
                                                                               check_if_is_template<tokenize_t>,
                                                                               check_if_is_template<content_t>,
                                                                               check_if_is_template<table_content_t>>,
                                                              T>;
#endif

    // ----

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept module_tag = requires {
        typename T::module_type;
        { T::name() } -> std::convertible_to<const char*>;
    };
#endif

    /** 
     *  Default traits of a "normal" virtual table.
     *  
     *  Particularly this means:
     *  - it is not a WITHOUT ROWID table (i.e. it has an implicit `rowid` column).
     *  - its definition is a `insertable_table_definition`
     *  
     *  Specific virtual table modules can specialize this struct to provide their own traits.
     */
    template<class M, class... Cs>
    struct virtual_table_traits {
        using module_type = M;
        using is_without_rowid = std::false_type;
        using definition_type = insertable_table_definition<Cs...>;
        using elements_type = elements_type_t<definition_type>;
        using omit_column_type = std::true_type;
    };

    /** 
     *  Encapsulates the intermediary (and temporary) `using_module<Object>(...)` expression.
     * 
     *  Implementation note: When making the virtual table this description is unpacked into the virtual table type itself.
     *  If desired or necessary one day, rename it to `virtual_table_definition`, and derive `virtual_table` from it, similar to
     *  `base_table` deriving from `base_table_definition`.
     */
    template<class O, class M, class... Cs>
    struct virtual_table_description : virtual_table_traits<M, Cs...>::definition_type {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        static_assert(module_tag<M>, "Template parameter M must be a module tag");
#endif
    };

    /**
     *  Represents an SQLite virtual table.
     */
    template<class O, class M, class... Cs>
    struct virtual_table : table_identifier, virtual_table_traits<M, Cs...>::definition_type {
        using traits_type = virtual_table_traits<M, Cs...>;
        using module_type = M;
        using object_type = O;
        using elements_type = typename traits_type::elements_type;
        using is_without_rowid = typename traits_type::is_without_rowid;
    };

    template<class T>
    inline constexpr bool is_virtual_table_v = polyfill::is_specialization_of_v<T, virtual_table>;

    template<class T>
    using is_virtual_table = polyfill::bool_constant<is_virtual_table_v<T>>;

#if SQLITE_VERSION_NUMBER >= 3009000
    struct fts5_module_tag {
        // simplify conceptual/meta programming
        using module_type = fts5_module_tag;

        static constexpr const char* name() {
            return "fts5";
        }
    };
#endif
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000
    /**
     *  Factory function for the FTS5 virtual table extension.
     *  
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::virtual_table_description<T, internal::fts5_module_tag, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_fts5_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }

    /**
     *  Factory function for the FTS5 virtual table extension.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<class T, class... Cs>
    internal::virtual_table_description<T, internal::fts5_module_tag, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_fts5_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for the FTS5 virtual table extension.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<orm_table_reference auto table, class... Cs>
    auto using_fts5(Cs... args) {
        return using_fts5<internal::auto_decay_table_ref_t<table>>(std::forward<Cs>(args)...);
    }
#endif
#endif

    /**
     *  Factory function for a virtual table definition.
     */
    template<class O, class M, class... Cs>
    internal::virtual_table<O, M, Cs...>
    make_virtual_table(std::string name, internal::virtual_table_description<O, M, Cs...> description) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), std::move(description)});
    }
}
