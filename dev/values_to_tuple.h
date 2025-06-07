#pragma once

#include <sqlite3.h>
#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::index_sequence, std::make_index_sequence
#include <tuple>  //  std::tuple, std::tuple_size, std::tuple_element
#endif

#include "row_extractor.h"
#include "arg_values.h"

namespace sqlite_orm::internal {

    template<class Tpl>
    struct tuple_from_values {
        SQLITE_ORM_STATIC_CALLOP Tpl operator()(sqlite3_value** values, int nValues) SQLITE_ORM_OR_CONST_CALLOP {
#ifdef SQLITE_ORM_CONTRACTS_SUPPORTED
            contract_assert(nValues == std::tuple_size<Tpl>::value);
#else
            (void)nValues;
#endif
            return tuple_from_values::create_from(values, std::make_index_sequence<std::tuple_size<Tpl>::value>{});
        }

      private:
        template<size_t... Idx>
        static Tpl create_from(sqlite3_value** values, std::index_sequence<Idx...>) {
            return {extract_boxed_value<std::tuple_element_t<Idx, Tpl>>(values[Idx])...};
        }
    };

    /*
     *  Explicit specialization for `arg_values`.
     */
    template<>
    struct tuple_from_values<std::tuple<arg_values>> {
        SQLITE_ORM_STATIC_CALLOP std::tuple<arg_values> operator()(sqlite3_value** values,
                                                                   int nValues) SQLITE_ORM_OR_CONST_CALLOP {
            return {arg_values(nValues, values)};
        }
    };
}
