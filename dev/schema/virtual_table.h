#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string>  //  std::string
#include <tuple>  //  std::tuple_element, std::make_tuple
#include <utility>  //  std::forward, std::move
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "table_base.h"
#include "column.h"

namespace sqlite_orm::internal {

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
    template<class... Cs, class T = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }

    template<class T, class... Cs>
    internal::fts5_module<T, Cs...> using_fts5(Cs... columns) {
        static_assert(polyfill::conjunction_v<internal::is_table_element_or_constraint<Cs>...>,
                      "Incorrect table elements or constraints");

        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::make_tuple(std::forward<Cs>(columns)...)});
    }
#endif

    template<class M>
    internal::virtual_table<M> make_virtual_table(std::string name, M module) {
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(return {std::move(name), std::move(module)});
    }
}
