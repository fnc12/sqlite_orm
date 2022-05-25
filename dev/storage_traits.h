#include <type_traits>  //  std::is_same, std::enable_if, std::true_type, std::false_type, std::integral_constant
#include <tuple>  //  std::tuple

#include "functional/cxx_universal.h"
#include "functional/cxx_polyfill.h"
#include "functional/mpl.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_transformer.h"
#include "type_traits.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        template<class... Ts>
        struct storage_impl;

        namespace storage_traits {

            template<class S, class O, class SFINAE = void>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v = false;
            template<class S, class O>
            SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v<S, O, polyfill::void_t<storage_pick_impl_t<S, O>>> = true;

            template<class S, class O>
            using is_mapped = polyfill::bool_constant<is_mapped_v<S, O>>;

            template<class S>
            struct storage_columns_count_impl : std::integral_constant<int, S::table_type::elements_count> {};

            template<>
            struct storage_columns_count_impl<storage_impl<>> : std::integral_constant<int, 0> {};

            /**
             *  S - storage
             *  O - mapped or not mapped data type
             */
            template<class S, class O>
            struct storage_columns_count : storage_columns_count_impl<storage_find_impl_t<S, O>> {};

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

            /**
             *  Table A `table_t<>`
             *  O is object type which is the reference target (e.g. foreign_key(&Visit::userId).references(&User::id) has O = User)
             */
            template<class Table, class O>
            struct table_foreign_keys_count
                : count_filtered_tuple<elements_type_t<Table>,
                                       check_if_is_type<O>::template fn,
                                       filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>,
                                       target_type_t> {};

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_foreign_keys_count_impl : std::integral_constant<int, 0> {};

#ifdef SQLITE_ORM_FOLD_EXPRESSIONS_SUPPORTED
            template<class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<Ts...>, O> {
                static constexpr int value = (table_foreign_keys_count<Ts, O>::value + ...);
            };
#else
            template<class H, class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<H, Ts...>, O> {
                static constexpr int value = table_foreign_keys_count<H, O>::value +
                                             storage_foreign_keys_count_impl<storage_impl<Ts...>, O>::value;
            };
#endif

            /**
             * S - storage class
             * O - type mapped to S
             * This class tells how many types mapped to S have foreign keys to O
             */
            template<class S, class O>
            struct storage_foreign_keys_count : storage_foreign_keys_count_impl<typename S::impl_type, O> {};

            template<class Table, class O>
            using table_foreign_keys_t =
                filter_tuple_t<elements_type_t<Table>,
                               check_if_is_type<O>::template fn,
                               target_type_t,
                               filter_tuple_sequence_t<elements_type_t<Table>, is_foreign_key>>;

            /*
             *  Implementation note: must be a struct instead of an template alias because the foreign keys tuple
             *  must be hoisted into a named alias, otherwise type replacement may fail for legacy compilers
             *  if an alias template has a dependent expression in it [SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION].
             */
            template<class Table, class O>
            struct table_fk_references {
                using foreign_keys = table_foreign_keys_t<Table, O>;

                using type = transform_tuple_t<foreign_keys, source_type_t>;
            };

            /**
             *  S - storage class
             *  O - type mapped to S
             */
            template<class S, class O>
            struct storage_fk_references_impl;

            template<class S, class O>
            struct storage_foreign_keys_impl;

            template<class... Ts, class O>
            struct storage_fk_references_impl<storage_impl<Ts...>, O>
                : conc_tuple<typename table_fk_references<Ts, O>::type...> {};

            template<class... Ts, class O>
            struct storage_foreign_keys_impl<storage_impl<Ts...>, O> : conc_tuple<table_foreign_keys_t<Ts, O>...> {};

            /**
             *  S - storage class
             *  O - type mapped to S
             *  type holds `std::tuple` with types that has references to O as  foreign keys
             */
            template<class S, class O>
            struct storage_fk_references : storage_fk_references_impl<typename S::impl_type, O> {};

            template<class S, class O>
            struct storage_foreign_keys : storage_foreign_keys_impl<typename S::impl_type, O> {};
        }
    }
}
