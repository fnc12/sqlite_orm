#pragma once

#include <tuple>  //  std::tuple

#include "functional/cxx_universal.h"
#include "functional/cxx_polyfill.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "type_traits.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        namespace storage_traits {

            template<class S, class O, class SFINAE = void>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v = false;
            template<class S, class O>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v<S, O, polyfill::void_t<storage_pick_impl_t<S, O>>> = true;

            template<class S, class O>
            using is_mapped = polyfill::bool_constant<is_mapped_v<S, O>>;

            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S>
            struct storage_mapped_columns_impl
                : tuple_transformer<filter_tuple_t<elements_type_t<table_type_t<S>>, is_column>, column_field_type_t> {
            };

            template<>
            struct storage_mapped_columns_impl<storage_impl<>> {
                using type = std::tuple<>;
            };

            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_mapped_columns : storage_mapped_columns_impl<storage_find_impl_t<S, O>> {};
        }
    }
}
