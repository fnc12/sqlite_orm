#pragma once

#include <tuple>  //  std::tuple, std::make_tuple, std::declval
#include <string>  //  std::string
#include <utility>  //  std::forward

#include "functional/cxx_universal.h"
#include "tuple_helper/tuple_filter.h"
#include "indexed_column.h"

namespace sqlite_orm {

    namespace internal {

        struct index_base {
            std::string name;
            bool unique = false;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
            index_base(std::string name, bool unique) : name{move(name)}, unique{unique} {}
#endif
        };

        template<class... Els>
        struct index_t : index_base {
            using elements_type = std::tuple<Els...>;
            using object_type = void;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            index_t(std::string name_, bool unique_, elements_type elements_) :
                index_base{move(name_), unique_}, elements(move(elements_)) {}
#endif

            elements_type elements;
        };
    }

    template<class... Cols>
    internal::index_t<decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                   Cols... cols) {
        using cols_tuple = std::tuple<Cols...>;
        static_assert(internal::count_tuple<cols_tuple, internal::is_where>::value <= 1,
                      "amount of where arguments can be 0 or 1");
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {move(name), false, std::make_tuple(internal::make_indexed_column(std::move(cols))...)});
    }

    template<class... Cols>
    internal::index_t<decltype(internal::make_indexed_column(std::declval<Cols>()))...>
    make_unique_index(std::string name, Cols... cols) {
        using cols_tuple = std::tuple<Cols...>;
        static_assert(internal::count_tuple<cols_tuple, internal::is_where>::value <= 1,
                      "amount of where arguments can be 0 or 1");
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {move(name), true, std::make_tuple(internal::make_indexed_column(std::move(cols))...)});
    }
}
