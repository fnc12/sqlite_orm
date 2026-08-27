#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::declval, std::tuple_element_t
#include <string>  //  std::string
#include <type_traits>  //  std::is_same
#include <utility>  //  std::forward
#endif

#include "../functional/cxx_type_traits_polyfill.h"
#include "../tuple_helper/tuple_traits.h"
#include "../table_type_of.h"
#include "../vocabulary/node_traits.h"
#include "../vocabulary/node_algorithms.h"  // is_statement_clause
#include "indexed_column.h"  // make_indexed_column

namespace sqlite_orm::internal {
    struct index_base {
        std::string name;
        bool unique = false;
    };

    template<class T, class... Els>
    struct index_t : index_base {
        using elements_type = std::tuple<Els...>;
        using object_type = void;
        using table_mapped_type = T;

        elements_type elements;
    };

    template<class T>
    constexpr bool is_index_v = polyfill::is_specialization_of_v<T, index_t>;

    /**
     *  Whether an index element belongs to the table the index is made for:
     *  an element that names a column must name a column of that table, while
     *  expressions and partial-index WHERE clauses carry no table type and are admitted.
     */
    template<class ColRef, class T, class SFINAE = void>
    constexpr bool is_index_element_of_v = true;

    template<class ColRef, class T>
    constexpr bool
        is_index_element_of_v<ColRef, T, std::enable_if_t<polyfill::is_detected<table_type_of_t, ColRef>::value>> =
            std::is_same<table_type_of_t<ColRef>, T>::value;

    template<class T, class... Cols>
    constexpr void validate_index_arguments() {
        static_assert(count_tuple<std::tuple<Cols...>, is_where>::value <= 1,
                      "amount of where arguments can be 0 or 1");
        static_assert(((is_where_v<Cols> || !is_statement_clause<Cols>::value) && ...),
                      "a make_index() argument must be an indexed column, an expression or a partial-index WHERE");
        static_assert((is_index_element_of_v<Cols, T> && ...),
                      "all indexed columns must belong to the table the index is made for");
    }
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T, class... Cols>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                      Cols... cols) {
        using namespace ::sqlite_orm::internal;
        validate_index_arguments<T, Cols...>();
        return {std::move(name), false, std::tuple{make_indexed_column(std::move(cols))...}};
    }

    template<class... Cols, class T = internal::table_type_of_t<std::tuple_element_t<0, std::tuple<Cols...>>>>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                      Cols... cols) {
        using namespace ::sqlite_orm::internal;
        validate_index_arguments<T, Cols...>();
        return {std::move(name), false, std::tuple{make_indexed_column(std::move(cols))...}};
    }

    template<class... Cols, class T = internal::table_type_of_t<std::tuple_element_t<0, std::tuple<Cols...>>>>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...>
    make_unique_index(std::string name, Cols... cols) {
        using namespace ::sqlite_orm::internal;
        validate_index_arguments<T, Cols...>();
        return {std::move(name), true, std::tuple{make_indexed_column(std::move(cols))...}};
    }
}
