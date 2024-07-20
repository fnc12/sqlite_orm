#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::enable_if, std::is_same, std::index_sequence, std::make_index_sequence
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element

#include "functional/cxx_functional_polyfill.h"
#include "type_traits.h"
#include "row_extractor.h"
#include "arg_values.h"

namespace sqlite_orm {

    namespace internal {

        template<class Tpl>
        struct tuple_from_values {
            template<class R = Tpl, satisfies_not<std::is_same, R, std::tuple<arg_values>> = true>
            R operator()(sqlite3_value** values, int /*argsCount*/) const {
                return this->create_from(values, std::make_index_sequence<std::tuple_size<Tpl>::value>{});
            }

            template<class R = Tpl, satisfies<std::is_same, R, std::tuple<arg_values>> = true>
            R operator()(sqlite3_value** values, int argsCount) const {
                return {arg_values(argsCount, values)};
            }

          private:
            template<size_t... Idx>
            Tpl create_from(sqlite3_value** values, std::index_sequence<Idx...>) const {
                return {this->extract<std::tuple_element_t<Idx, Tpl>>(values[Idx])...};
            }

            template<class T>
            T extract(sqlite3_value* value) const {
                const auto rowExtractor = boxed_value_extractor<T>();
                return rowExtractor.extract(value);
            }
        };
    }
}
