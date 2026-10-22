#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  // std::convertible_to
#endif
#include <type_traits>  //  std::is_base_of, std::conditional_t, std::false_type, std::true_type
#include <string>  //  std::string
#include <tuple>  //  std::make_tuple, std::tuple_cat, std::apply
#include <utility>  //  std::move, std::forward
#endif

#include "../functional/cxx_polyfill/type_traits.h"
#include "../functional/gsl.h"
#include "../functional/type_traits.h"
#include "../vocabulary/node_traits.h"  //  orm_table_reference, auto_decay_table_ref_t
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "table_base.h"
#include "column.h"  //  make_reflected_columns, make_reflected_hidden_columns
#include "dbo_name.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept module_tag = requires {
        typename T::module_type;
        { T::name() } -> std::convertible_to<orm_gsl::czstring>;
    };
#endif

    /**
     *  Base class of the module tags of built-in eponymous virtual tables.
     *
     *  The definition of eponymous virtual tables is built-in, fixed and implicit,
     *  and they can only be created with optional table-values for their hidden columns.
     */
    struct eponymous_module {};

    /** 
     *  Default base traits of a virtual table module.
     *  
     *  Particularly this means:
     *  - It is eponymous if the module tag derives from `eponymous_module`.
     *  - It is not a WITHOUT ROWID table (i.e. it has an implicit `rowid` column).
     *  - Omits the column type in the SQL creation statement.
     *  
     *  Specific virtual table modules can specialize this struct to provide their own traits.
     */
    template<class M>
    struct virtual_table_module_traits {
        using module_type = M;
        using is_eponymous = std::is_base_of<eponymous_module, M>;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    /** 
     *  Default traits of a virtual table.
     *  
     *  Particularly this means:
     *  - Its definition is a read-only `table_definition` for an eponymous module,
     *    an `insertable_table_definition` otherwise.
     *  
     *  Specific virtual table modules can specialize this struct to provide their own traits.
     */
    template<class M, class... Cs>
    struct virtual_table_traits : virtual_table_module_traits<M> {
        using definition_type = std::conditional_t<virtual_table_module_traits<M>::is_eponymous::value,
                                                   table_definition<Cs...>,
                                                   insertable_table_definition<Cs...>>;
        using elements_type = elements_type_t<definition_type>;
    };

    /** 
     *  Encapsulates the intermediary (and temporary) (and deprecated) `using_module<Object>(...)` expression.
     */
    template<class O, class M, class... Cs>
    struct virtual_table_description : virtual_table_traits<M, Cs...>::definition_type {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        static_assert(module_tag<M>, "Template parameter M must be a module tag");
#endif
    };

    /** 
     *  Encapsulates the intermediary (and temporary) `using_module(...)` expression.
     *  
     *  Implementation note: When making the virtual table this virtual table definition is unpacked into the virtual table type itself.
     *  If desired or necessary one day, derive `virtual_table` from it, similar to `base_table` deriving from `base_table_definition`.
     */
    template<class M, class... Cs>
    struct virtual_table_definition : virtual_table_traits<M, Cs...>::definition_type {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
        static_assert(module_tag<M>, "Template parameter M must be a module tag");
#endif
        using module_type = M;
    };

    /**
     *  Represents an SQLite virtual table.
     */
    template<class O, class M, class... Cs>
    struct virtual_table : table_identifier, virtual_table_traits<M, Cs...>::definition_type {
        using traits_type = virtual_table_traits<M, Cs...>;
        using module_traits_type = virtual_table_module_traits<M>;
        using module_type = M;
        using object_type = O;
        using elements_type = typename traits_type::elements_type;
        using is_without_rowid = typename traits_type::is_without_rowid;
    };

    template<class T>
    constexpr bool is_virtual_table_v = polyfill::is_specialization_of_v<T, virtual_table>;

    /**
     *  Factory function for a virtual table definition of module M.
     */
    template<class M, class... Cs>
    virtual_table_definition<M, Cs...> make_virtual_table_definition(Cs... elements) {
        return {{std::make_tuple(std::move(elements)...)}};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a virtual table.
     *  
     *  [Deprecation notice] This factory function is deprecated and will be removed in v1.11.
     */
    template<class O, class M, class... Cs>
    internal::virtual_table<O, M, Cs...>
    make_virtual_table(std::string name, internal::virtual_table_description<O, M, Cs...> description) {
        return {std::move(name), std::move(description)};
    }

    /**
     *  Factory function for a virtual table.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<class O, class M, class... Cs>
    internal::virtual_table<O, M, Cs...> make_virtual_table(std::string name,
                                                            internal::virtual_table_definition<M, Cs...> definition) {
        return {std::move(name), std::move(definition)};
    }

    /**
     *  Factory function for a virtual table.
     *  
     *  The mapped object type is determined implicitly from the first column definition.
     */
    template<class M, class... Cs, class O = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    internal::virtual_table<O, M, Cs...> make_virtual_table(std::string name,
                                                            internal::virtual_table_definition<M, Cs...> definition) {
        return make_virtual_table<O>(std::move(name), std::move(definition));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a virtual table.
     *  
     *  The mapped object type is explicitly specified.
     */
    template<orm_table_reference auto table, class M, class... Cs>
    auto make_virtual_table(std::string name, internal::virtual_table_definition<M, Cs...> definition) {
        return make_virtual_table<internal::auto_decay_table_ref_t<table>>(std::move(name), std::move(definition));
    }
#endif
}

#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    /**
     *  Factory function for a virtual table definition of module M, mapping the data structure O:
     *  the columns of O's non-static data members, the hidden columns of the non-static data members of
     *  O's nested `hidden` struct, followed by `elements`.
     */
    template<class M, class O, class... Es>
    auto make_reflected_virtual_table_definition(Es... elements) {
        return std::apply(
            [](auto&&... definition) static {
                return make_virtual_table_definition<M>(std::forward<decltype(definition)>(definition)...);
            },
            std::tuple_cat(make_reflected_columns<O>(),
                           make_reflected_hidden_columns<O>(),
                           std::tuple<Es...>{std::move(elements)...}));
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a virtual table.
     *
     *  The mapped object type is determined implicitly from the first column definition.
     *  An eponymous virtual table is named after its module. Otherwise the optional `[[=orm_name("…")]]`
     *  annotation of the mapped object type supplies the table name (otherwise the type's reflected identifier is used).
     *
     *  Example: `make_virtual_table(using_fts5<Post>(tokenize("porter")))`
     */
    template<class M, class... Cs, class O = typename std::tuple_element_t<0, std::tuple<Cs...>>::object_type>
    auto make_virtual_table(internal::virtual_table_definition<M, Cs...> definition) {
        using namespace ::sqlite_orm::internal;

        std::string name;
        if constexpr (virtual_table_module_traits<M>::is_eponymous::value) {
            name = M::name();
        } else {
            name = resolve_dbo_name<O>(extract_type_annotations<O>());
        }
        // unqualified: module-specific factories (fts5) are found by argument-dependent lookup
        return make_virtual_table<O>(std::move(name), std::move(definition));
    }
}
#endif
