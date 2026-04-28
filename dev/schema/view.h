#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_VIEW
#include <type_traits>  //  std::remove_cvref
#include <utility>  // std::forward, std::move, std::index_sequence, std::make_index_sequence
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/meta_util.h"
#include "../column_pointer.h"
#include "../select_constraints.h"
#include "column.h"
#include "table_base.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_WITH_VIEW
    /**
     *  View definition, mapping an aggregate object type to a corresponding select statement.
     */
    template<class O, class Select, class... Cs>
    struct query_view : table_identifier, table_definition<Cs...> {
        using definition_base_type = table_definition<Cs...>;
        using object_type = O;
        using elements_type = typename definition_base_type::elements_type;
        using select_type = Select;

        select_type select;
    };

    template<class T>
    inline constexpr bool is_view_v = polyfill::is_specialization_of_v<T, query_view>;
#else
    template<class T>
    inline constexpr bool is_view_v = false;
#endif

    template<class T>
    using is_view = polyfill::bool_constant<is_view_v<T>>;
}

#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    template<class O, class Select, size_t... I>
    auto make_view(std::string name, std::index_sequence<I...>, Select select) {
        constexpr auto memberNames = extract_member_names<O>();
        constexpr auto memberPointers = extract_member_pointers<O>();

        using view_type =
            query_view<O,
                       Select,
                       decltype(make_column(std::string(std::get<I>(memberNames)), std::get<I>(memberPointers)))...>;

        return view_type{std::move(name),
                         std::tuple{make_column(std::string(std::get<I>(memberNames)), std::get<I>(memberPointers))...},
                         std::move(select)};
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a view definition.
     *  
     *  The mapped object type is explicitly specified, columns and their names are deferred from the object type.
     *  The object type must be an aggregate.
     */
    template<class O, class Select>
        requires (internal::is_select_expression_v<Select>)
    auto make_view(std::string name, Select select) {
        using namespace ::sqlite_orm::internal;

        if constexpr (is_select_v<Select>) {
            select.highest_level = true;
        }
        return internal::make_view<O>(std::move(name),
                                      std::make_index_sequence<count_members<O>()>{},
                                      std::move(select));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a view definition.
     *  
     *  The mapped object type is explicitly specified, columns and their names are deferred from the object type.
     *  The object type must be an aggregate.
     */
    template<orm_table_reference auto table, class Select>
    auto make_view(std::string name, Select select) {
        return make_view<internal::auto_decay_table_ref_t<table>>(std::move(name), std::forward<Select>(select));
    }
#endif
}
#endif
#endif
