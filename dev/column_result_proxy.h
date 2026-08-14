#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "tuple_helper/tuple_transformer.h"
#include "table_reference.h"

namespace sqlite_orm::internal {
    /*
     *  Holder for the type of an unmapped aggregate/structure/object to be constructed ad-hoc from column results.
     *  `T` must be constructible using direct-list-initialization.
     */
    template<class T, class ColResults>
    struct structure {
        using type = T;
    };

    /*
     *  Determine the actual and final result type of an intermediate column result produced by `column_result_t`,
     *  unwrapping `table_reference` and `structure`, and transforming tuples element-wise.
     */
    template<class T, class SFINAE = void>
    struct column_result_proxy : std::remove_const<T> {};

    template<class T>
    using column_result_proxy_t = typename column_result_proxy<T>::type;

    /*
     *  Unwrap `table_reference`, `structure`.
     */
    template<class P>
    struct column_result_proxy<
        P,
        std::enable_if_t<polyfill::disjunction_v<is_table_reference<P>, polyfill::is_specialization_of<P, structure>>>>
        : P {};

    /*
     *  Calculate result of multiple columns.
     */
    template<class Tpl>
    struct column_result_proxy<Tpl, match_specialization_of<Tpl, std::tuple>>
        : tuple_transformer<Tpl, column_result_proxy_t> {};
}
