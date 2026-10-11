#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
#include <string>  // std::string
#include <tuple>  // std::tuple, std::tuple_size_v
#include <utility>  // std::forward, std::move, std::index_sequence, std::make_index_sequence
#include <meta>  // std::meta::info
#endif
#endif
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../functional/meta_util.h"
#include "../tuple_helper/tuple_traits.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "column.h"  // sqlite_orm::make_column
#include "table_base.h"
#include "orm_name.h"

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
    constexpr bool is_view_v = polyfill::is_specialization_of_v<T, query_view>;
#else
    template<class T>
    constexpr bool is_view_v = false;
#endif
}

#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
namespace sqlite_orm::internal {
    /**
     *  Returns the mapped name of the entity `refl`, a view's object type or one of its non-static data members.
     *  A view takes no constraints, hence a mapped name is the only annotation of sqlite_orm's it accepts.
     */
    template<std::meta::info refl>
    consteval orm_gsl::czstring view_mapped_name_of() {
        using annotations_type = decltype(extract_orm_annotations<refl>());
        static_assert(count_tuple<annotations_type, is_mapped_name_literal>::value ==
                          std::tuple_size_v<annotations_type>,
                      "A view and its columns can only be annotated with a mapped name");

        return mapped_name_of<refl>();
    }

    template<class O, class Select>
    auto make_reflected_view(Select select) {
        std::string viewName{view_mapped_name_of<^^O>()};
        static /*gcc*/ constexpr auto members = extract_members<O>();

        auto columns = []<size_t... I>(std::index_sequence<I...>) static {
            return std::tuple{sqlite_orm::make_column(std::string{view_mapped_name_of<members[I]>()},
                                                      splice_member_pointer<members[I]>())...};
        }(std::make_index_sequence<members.size()>{});

        return [&viewName, &select]<class... Cs>(std::tuple<Cs...>&& cols) {
            return query_view<O, Select, Cs...>{std::move(viewName), std::move(cols), std::move(select)};
        }(std::move(columns));
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    /**
     *  Factory function for a view definition.
     *
     *  The mapped object type is explicitly specified, columns and their names are deferred from the object type.
     *  The object type must be an aggregate. The optional `[[="…"_orm_name]]` class-scope annotation overrides
     *  the view name (otherwise the type's reflected identifier is used). Likewise,
     *  a `[[="…"_orm_name]]` member annotation overrides the column name; view columns take no constraints.
     *  Annotations whose type is not declared within namespace `sqlite_orm` are ignored.
     */
    template<class O, class Select>
        requires (internal::is_select_expression_v<Select>)
    auto make_view(Select select) {
        using namespace ::sqlite_orm::internal;

        if constexpr (is_select_v<Select>) {
            select.highest_level = true;
        }
        return make_reflected_view<O>(std::move(select));
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Factory function for a view definition.
     *
     *  The mapped object type is explicitly specified, columns and their names are deferred from the object type.
     *  The object type must be an aggregate. The optional `[[="…"_orm_name]]` class-scope annotation overrides
     *  the view name (otherwise the type's reflected identifier is used). Likewise,
     *  a `[[="…"_orm_name]]` member annotation overrides the column name; view columns take no constraints.
     *  Annotations whose type is not declared within namespace `sqlite_orm` are ignored.
     */
    template<orm_table_reference auto table, class Select>
    auto make_view(Select select) {
        return make_view<internal::auto_decay_table_ref_t<table>>(std::forward<Select>(select));
    }
#endif
}
#endif
#endif
