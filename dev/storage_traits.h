#include <type_traits>  //  std::is_same, std::enable_if, std::true_type, std::false_type, std::integral_constant
#include <tuple>  //  std::tuple

namespace sqlite_orm {

    namespace internal {

        template<class... Ts>
        struct storage_impl;

        template<class T, class... Args>
        struct table_t;

        template<class A, class B>
        struct foreign_key_t;

        //        template<class T, class SFINAE = void>
        //        struct table_type;

        namespace storage_traits {

            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S, class T, class SFINAE = void>
            struct type_is_mapped_impl;

            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class T>
            struct type_is_mapped : type_is_mapped_impl<typename S::impl_type, T> {};

            /**
             *  Final specialisation
             */
            template<class T>
            struct type_is_mapped_impl<storage_impl<>, T, void> : std::false_type {};

            template<class S, class T>
            struct type_is_mapped_impl<
                S,
                T,
                typename std::enable_if<std::is_same<T, typename S::table_type::object_type>::value>::type>
                : std::true_type {};

            template<class S, class T>
            struct type_is_mapped_impl<
                S,
                T,
                typename std::enable_if<!std::is_same<T, typename S::table_type::object_type>::value>::type>
                : type_is_mapped_impl<typename S::super, T> {};

            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S, class T, class SFINAE = void>
            struct storage_columns_count_impl;

            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class T>
            struct storage_columns_count : storage_columns_count_impl<typename S::impl_type, T> {};

            /**
             *  Final specialisation
             */
            template<class T>
            struct storage_columns_count_impl<storage_impl<>, T, void> : std::integral_constant<int, 0> {};

            template<class S, class T>
            struct storage_columns_count_impl<
                S,
                T,
                typename std::enable_if<std::is_same<T, typename S::table_type::object_type>::value>::type>
                : std::integral_constant<int, S::table_type::columns_count> {};

            template<class S, class T>
            struct storage_columns_count_impl<
                S,
                T,
                typename std::enable_if<!std::is_same<T, typename S::table_type::object_type>::value>::type>
                : storage_columns_count_impl<typename S::super, T> {};

            /**
             *  T - table type.
             */
            template<class T>
            struct table_types;

            /**
             *  type is std::tuple of field types of mapped colums.
             */
            template<class T, class... Args>
            struct table_types<table_t<T, Args...>> {
                using type = std::tuple<typename Args::field_type...>;
            };

            /**
             *  S - storage_impl type
             *  T - mapped or not mapped data type
             */
            template<class S, class T, class SFINAE = void>
            struct storage_mapped_columns_impl;

            /**
             *  S - storage
             *  T - mapped or not mapped data type
             */
            template<class S, class T>
            struct storage_mapped_columns : storage_mapped_columns_impl<typename S::impl_type, T> {};

            /**
             *  Final specialisation
             */
            template<class T>
            struct storage_mapped_columns_impl<storage_impl<>, T, void> {
                using type = std::tuple<>;
            };

            template<class S, class T>
            struct storage_mapped_columns_impl<
                S,
                T,
                typename std::enable_if<std::is_same<T, typename S::table_type::object_type>::value>::type> {
                using table_type = typename S::table_type;
                using type = typename table_types<table_type>::type;
            };

            template<class S, class T>
            struct storage_mapped_columns_impl<
                S,
                T,
                typename std::enable_if<!std::is_same<T, typename S::table_type::object_type>::value>::type>
                : storage_mapped_columns_impl<typename S::super, T> {};

            template<class C, class O>
            struct column_foreign_keys_count : std::integral_constant<int, 0> {};

            template<class A, class B, class O>
            struct column_foreign_keys_count<foreign_key_t<A, B>, O> {
                //                static constexpr const int value = std::is_same<O, >::value;
            };

            template<class O, class... Cs>
            struct table_foreign_keys_count_impl;

            template<class O>
            struct table_foreign_keys_count_impl<O> {
                static constexpr const int value = 0;
            };

            template<class O, class H, class... Tail>
            struct table_foreign_keys_count_impl<O, H, Tail...> {
                static constexpr const int value = 0;
            };

            template<class T, class O>
            struct table_foreign_keys_count;

            template<class S, class O>
            struct storage_foreign_keys_count_impl;

            template<class O>
            struct storage_foreign_keys_count_impl<storage_impl<>, O> : std::integral_constant<int, 0> {};

            template<class H, class... Ts, class O>
            struct storage_foreign_keys_count_impl<storage_impl<H, Ts...>, O> {
                //            static constexpr const int value =
            };

            /**
         * S - storage class
         * O - type mapped to S
         * This class tells how many types mapped to S have foreign keys to O
         */
            template<class S, class O>
            struct storage_foreign_keys_count {};

        }
    }
}
