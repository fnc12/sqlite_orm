#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple_element, std::make_tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/mpl.h"
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

    template<class M>
    struct virtual_table : table_base, M {
        using module_type = M;
        using object_type = typename module_type::object_type;
        using elements_type = typename module_type::elements_type;

        static constexpr bool is_without_rowid_v = false;
        using is_without_rowid = polyfill::bool_constant<is_without_rowid_v>;

        const module_type& module() const {
            return *this;
        }
    };

#if SQLITE_VERSION_NUMBER >= 3009000
    template<class T, class... Cs>
    struct fts5_module : mapped_columns_mixin<Cs...> {
        using base_type = mapped_columns_mixin<Cs...>;
        using object_type = T;
        using elements_type = typename base_type::elements_type;
    };
#endif

    template<class M, class G, class S>
    bool exists_in_composite_primary_key(const virtual_table<M>& /*virtualTable*/,
                                         const column_field<G, S>& /*column*/) {
        return false;
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#if SQLITE_VERSION_NUMBER >= 3009000
    /**
     *  Factory function for the FTS5 virtual table extension.
     *  
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
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
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
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
    template<class M>
    internal::virtual_table<M> make_virtual_table(std::string name, M module) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), std::move(module)});
    }
}
