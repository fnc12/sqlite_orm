#pragma once

#include <sqlite3.h>
#include <type_traits>  //  std::index_sequence, std::make_index_sequence
#include <tuple>  //  std::tuple, std::tuple_size, std::get

#include "functional/cxx_universal.h"
#include "row_extractor.h"
#include "arg_values.h"

namespace sqlite_orm {

    namespace internal {

        struct values_to_tuple {
            template<class Tpl>
            void operator()(sqlite3_value** values, Tpl& tuple, int /*argsCount*/) const {
                (*this)(values, tuple, std::make_index_sequence<std::tuple_size<Tpl>::value>{});
            }

            void operator()(sqlite3_value** values, std::tuple<arg_values>& tuple, int argsCount) const {
                std::get<0>(tuple) = arg_values(argsCount, values);
            }

          private:
#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<class Tpl, size_t... Idx>
            void operator()(sqlite3_value** values, Tpl& tuple, std::index_sequence<Idx...>) const {
                (this->extract(values[Idx], std::get<Idx>(tuple)), ...);
            }
#else
            template<class Tpl, size_t I, size_t... Idx>
            void operator()(sqlite3_value** values, Tpl& tuple, std::index_sequence<I, Idx...>) const {
                this->extract(values[I], std::get<I>(tuple));
                (*this)(values, tuple, std::index_sequence<Idx...>{});
            }
            template<class Tpl, size_t... Idx>
            void operator()(sqlite3_value** /*values*/, Tpl&, std::index_sequence<Idx...>) const {}
#endif
            template<class T>
            void extract(sqlite3_value* value, T& t) const {
                t = row_extractor<T>{}.extract(value);
            }
        };
    }
}
