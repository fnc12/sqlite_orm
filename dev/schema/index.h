#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <tuple>  //  std::tuple, std::declval, std::tuple_element_t
#include <string>  //  std::string
#include <utility>  //  std::forward
#endif

#include "../tuple_helper/tuple_traits.h"
#include "../functional/mpl.h"
#include "../table_type_of.h"
#include "../vocabulary/node_traits.h"
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
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
    template<class T, class... Cols>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                      Cols... cols) {
        using namespace ::sqlite_orm::internal;
        using cols_tuple = mpl::pack<Cols...>;
        static_assert(count_tuple<cols_tuple, is_where>::value <= 1, "amount of where arguments can be 0 or 1");
        return {std::move(name), false, std::tuple{make_indexed_column(std::move(cols))...}};
    }

    template<class... Cols, class T = internal::table_type_of_t<std::tuple_element_t<0, std::tuple<Cols...>>>>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                      Cols... cols) {
        using namespace ::sqlite_orm::internal;
        using cols_tuple = std::tuple<Cols...>;
        static_assert(count_tuple<cols_tuple, is_where>::value <= 1, "amount of where arguments can be 0 or 1");
        return {std::move(name), false, std::tuple{make_indexed_column(std::move(cols))...}};
    }

    template<class... Cols, class T = internal::table_type_of_t<std::tuple_element_t<0, std::tuple<Cols...>>>>
    internal::index_t<T, decltype(internal::make_indexed_column(std::declval<Cols>()))...>
    make_unique_index(std::string name, Cols... cols) {
        using namespace ::sqlite_orm::internal;
        using cols_tuple = std::tuple<Cols...>;
        static_assert(count_tuple<cols_tuple, is_where>::value <= 1, "amount of where arguments can be 0 or 1");
        return {std::move(name), true, std::tuple{make_indexed_column(std::move(cols))...}};
    }
}
