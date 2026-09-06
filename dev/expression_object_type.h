#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_reference
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/type_traits.h"
#include "vocabulary/node_traits.h"
#include "prepared_statement.h"

namespace sqlite_orm::internal {
    template<class T, class SFINAE = void>
    struct expression_object_type;

    template<class T>
    using expression_object_type_t = typename expression_object_type<T>::type;

    template<typename S>
    using statement_object_type_t = expression_object_type_t<expression_type_t<std::remove_reference_t<S>>>;

    template<class T>
    struct expression_object_type<update_t<T>, void> : value_unref_type<T> {};

    template<class T>
    struct expression_object_type<replace_t<T>, void> : value_unref_type<T> {};

    template<class T>
    struct expression_object_type<T, match_if<is_replace_range, T>> {
        using type = object_type_t<T>;
    };

    template<class T, class... Ids>
    struct expression_object_type<remove_t<T, Ids...>, void> : value_unref_type<T> {};

    template<class T>
    struct expression_object_type<insert_t<T>, void> : value_unref_type<T> {};

    template<class T>
    struct expression_object_type<T, match_if<is_insert_range, T>> {
        using type = object_type_t<T>;
    };

    template<class T, class... Cols>
    struct expression_object_type<insert_explicit<T, Cols...>, void> : value_unref_type<T> {};

    template<class DML>
    decltype(auto) access_dml_object(DML& statement) {
        return forward_lvalue_ref(statement.expression.object);
    }
}
