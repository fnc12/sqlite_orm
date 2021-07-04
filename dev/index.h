#pragma once

#include <tuple>  //  std::tuple, std::make_tuple
#include <string>  //  std::string
#include <utility>  //  std::forward

#include "indexed_column.h"

namespace sqlite_orm {

    namespace internal {

        struct index_base {
            std::string name;
            bool unique = false;
        };

        template<class... Cols>
        struct index_t : index_base {
            using columns_type = std::tuple<Cols...>;
            using object_type = void;

            index_t(std::string name_, bool unique_, columns_type columns_) :
                index_base{move(name_), unique_}, columns(move(columns_)) {}

            columns_type columns;
        };
    }

    template<class... Cols>
    internal::index_t<typename internal::indexed_column_maker<Cols>::type...> make_index(const std::string& name,
                                                                                         Cols... cols) {
        return {name, false, std::make_tuple(internal::make_indexed_column(cols)...)};
    }

    template<class... Cols>
    internal::index_t<typename internal::indexed_column_maker<Cols>::type...> make_unique_index(const std::string& name,
                                                                                                Cols... cols) {
        return {name, true, std::make_tuple(internal::make_indexed_column(cols)...)};
    }
}
