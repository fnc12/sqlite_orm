#pragma once

#include <type_traits>  //  std::declval
#include <string>  //  std::string
#include <utility>  //  std::forward

#include "functional/cxx_universal.h"
#include "functional/tuple.h"
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
            using elements_type = mpl::tuple<Els...>;
            using object_type = void;

#ifndef SQLITE_ORM_AGGREGATE_BASES_SUPPORTED
            index_t(std::string name_, bool unique_, elements_type elements_) :
                index_base{move(name_), unique_}, elements(std::move(elements_)) {}
#endif

            elements_type elements;
        };
    }

    template<class... Cols>
    internal::index_t<decltype(internal::make_indexed_column(std::declval<Cols>()))...> make_index(std::string name,
                                                                                                   Cols... cols) {
        static_assert(internal::count_tuple<mpl::pack<Cols...>, internal::is_where>::value <= 1,
                      "amount of where arguments can be 0 or 1");
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {move(name), false, mpl::make_tuple(internal::make_indexed_column(std::move(cols))...)});
    }

    template<class... Cols>
    internal::index_t<decltype(internal::make_indexed_column(std::declval<Cols>()))...>
    make_unique_index(std::string name, Cols... cols) {
        static_assert(internal::count_tuple<mpl::pack<Cols...>, internal::is_where>::value <= 1,
                      "amount of where arguments can be 0 or 1");
        SQLITE_ORM_CLANG_SUPPRESS_MISSING_BRACES(
            return {move(name), true, mpl::make_tuple(internal::make_indexed_column(std::move(cols))...)});
    }
}
