#pragma once

#include <sqlite3.h>
#include <tuple>  //  std::get, std::tuple_element

#include "row_extractor.h"
#include "arg_values.h"

namespace sqlite_orm {

    namespace internal {

        /**
         *  T is a std::tuple type
         *  I is index to extract value from values C array to tuple. I must me < std::tuple_size<T>::value
         */
        template<class T, int I>
        struct values_to_tuple {

            void extract(sqlite3_value **values, T &tuple, int argsCount) const {
                using element_type = typename std::tuple_element<I, T>::type;
                std::get<I>(tuple) = row_extractor<element_type>().extract(values[I]);

                values_to_tuple<T, I - 1>().extract(values, tuple, argsCount);
            }
        };

        template<class T>
        struct values_to_tuple<T, -1> {
            void extract(sqlite3_value **values, T &tuple, int argsCount) const {
                //..
            }
        };

        template<>
        struct values_to_tuple<std::tuple<arg_values>, 0> {
            void extract(sqlite3_value **values, std::tuple<arg_values> &tuple, int argsCount) const {
                std::get<0>(tuple) = arg_values(argsCount, values);
            }
        };
    }
}
