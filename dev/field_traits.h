#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::enable_if, std::is_same
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>  // std::same_as
#endif
#endif

#include "member_traits/member_traits.h"
#include "type_traits.h"
#include "field_of.h"

namespace sqlite_orm::internal {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    template<class Hidden, class F, class VTabStruct>
    concept hidden_field_of_vtab =
        is_field_of_v<F Hidden::*, typename VTabStruct::hidden::template _of<typename Hidden::enclosing_type>>;

    template<class CP, class VTabStruct>
    concept hidden_column_of_vtab = std::same_as<member_object_type_t<field_type_t<CP>>, typename VTabStruct::hidden>;
#else
    template<class CP, class VTabStruct, class SFINAE = void>
    inline constexpr bool is_hidden_column_of_vtab_v = false;

    template<class CP, class VTabStruct>
    inline constexpr bool is_hidden_column_of_vtab_v<
        CP,
        VTabStruct,
        std::enable_if_t<std::is_same<member_object_type_t<field_type_t<CP>>, typename VTabStruct::hidden>::value>> =
        true;

    template<class Hidden, class F, class VTabStruct>
    inline constexpr bool is_hidden_field_of_vtab_v =
        is_field_of_v<F Hidden::*, typename VTabStruct::hidden::template _of<typename Hidden::enclosing_type>>;
#endif
}
