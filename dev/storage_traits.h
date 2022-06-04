#pragma once

#include <tuple>  //  std::tuple

#include "functional/cxx_type_traits_polyfill.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "type_traits.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        namespace storage_traits {

            /**
             *  S - schema_objects type
             *  T - mapped or not mapped data type
             */
            template<class S>
            struct storage_mapped_columns_impl
                : tuple_transformer<filter_tuple_t<elements_type_t<S>, is_column>, field_type_t> {};

            template<>
            struct storage_mapped_columns_impl<polyfill::nonesuch> {
                using type = std::tuple<>;
            };

            /**
             *  S - schema_objects type
             *  T - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_mapped_columns : storage_mapped_columns_impl<storage_find_table_t<O, S>> {};
        }
    }
}
