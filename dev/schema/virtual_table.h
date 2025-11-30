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
#include "../functional/gsl.h"
#include "../functional/mpl.h"
#include "../type_traits.h"
#include "../constraints.h"
#include "table_base.h"
#include "column.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class T>
    concept module_tag = requires {
        typename T::module_type;
        { T::name() } -> std::convertible_to<orm_gsl::czstring>;
    };
#endif

    /** 
     *  Default base traits of a "normal" virtual table module.
     *  
     *  Particularly this means:
     *  - It is not eponymous.
     *    The definition of eponymous virtual tables is built-in, fixed and implicit,
     *    and they can only be created with optional table-values for their hidden columns.
     *  - It is not a WITHOUT ROWID table (i.e. it has an implicit `rowid` column).
     *  - Omits the column type in the SQL creation statement.
     *  
     *  Specific virtual table modules can specialize this struct to provide their own traits.
     */
    template<class M>
    struct virtual_table_module_traits {
        using module_type = M;
        using is_eponymous = std::false_type;
        using is_without_rowid = std::false_type;
        using omit_column_type = std::true_type;
    };

    /** 
     *  Default traits of a "normal" virtual table.
     *  
     *  Particularly this means :
     *  - Its definition is a `insertable_table_definition`.
     *  
     *  Specific virtual table modules can specialize this struct to provide their own traits.
     */
    template<class M, class... Cs>
    struct virtual_table_traits : virtual_table_module_traits<M> {
        using definition_type = insertable_table_definition<Cs...>;
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
    inline constexpr bool is_virtual_table_v = polyfill::is_specialization_of_v<T, virtual_table>;

    template<class T>
    using is_virtual_table = polyfill::bool_constant<is_virtual_table_v<T>>;
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
