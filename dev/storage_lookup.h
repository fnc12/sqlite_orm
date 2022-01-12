#pragma once

#include <type_traits>

#include "type_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class... Ts>
        struct storage_t;

        template<class... Ts>
        struct storage_impl;

        template<class T>
        struct is_storage : std::false_type {};

        template<class... Ts>
        struct is_storage<storage_t<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage<const storage_t<Ts...>> : std::true_type {};

        template<class T>
        struct is_storage_impl : std::false_type {};

        template<class... Ts>
        struct is_storage_impl<storage_impl<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage_impl<const storage_impl<Ts...>> : std::true_type {};

        /**
         *  A mapped data type's CTE alias by which a CTE table descriptor can be found.
         */
        template<typename O>
        using cte_label_t = typename O::cte_label_type;

        /**
         *  A data type's CTE alias, otherwise T itself is used as a CTE alias.
         */
        template<typename T>
        using detected_cte_label_t = polyfill::detected_or_t<T, cte_label_t, T>;
    }

    namespace internal {

        template<typename S, typename O>
        using object_type_matches =
            typename std::conjunction<std::is_void<typename S::table_type::cte_label_type>,
                                      std::is_same<O, typename S::table_type::object_type>>::type;
        template<typename S, typename Label>
        using label_type_matches = typename std::conjunction<
            std::negation<std::is_void<typename S::table_type::cte_label_type>>,
            std::is_same<detected_cte_label_t<Label>, typename S::table_type::cte_label_type>>::type;
        template<typename S, typename Lookup>
        using lookup_type_matches =
            typename std::disjunction<object_type_matches<S, Lookup>, label_type_matches<S, Lookup>>::type;

        /**
         *  S - storage_impl type
         *  O - mapped data type or CTE label
         */
        template<class S, class O, class SFINAE = void>
        struct storage_pick_impl_type;

        template<class S, class O>
        struct storage_pick_impl_type<S, O, match_if<lookup_type_matches, S, O>> {
            using type = S;
        };

        template<class S, class O>
        struct storage_pick_impl_type<S, O, match_if_not<lookup_type_matches, S, O>>
            : storage_pick_impl_type<typename S::super, O> {};

        /**
         *  Final specialisation
         */
        template<class O>
        struct storage_pick_impl_type<storage_impl<>, O, void> {};

        /**
         *  storage_t specialization
         */
        template<class... Ts, class O>
        struct storage_pick_impl_type<storage_t<Ts...>, O, void>
            : storage_pick_impl_type<typename storage_t<Ts...>::impl_type, O> {};

        /**
         *  S - storage_t or storage_impl type
         *  Lookup - mapped data type or CTE label
         */
        template<class S, class Lookup>
        using storage_pick_impl_t =
            std::conditional_t<std::is_const<S>::value,
                               std::add_const_t<typename storage_pick_impl_type<std::remove_const_t<S>, Lookup>::type>,
                               typename storage_pick_impl_type<S, Lookup>::type>;

        /**
         *  S - storage_impl type
         *  O - mapped data type or CTE label
         */
        template<class S, class O, class SFINAE = void>
        struct storage_find_impl_type;

        template<class S, class O>
        struct storage_find_impl_type<S, O, match_if<lookup_type_matches, S, O>> {
            using type = S;
        };

        template<class S, class O>
        struct storage_find_impl_type<S, O, match_if_not<lookup_type_matches, S, O>>
            : storage_find_impl_type<typename S::super, O> {};

        /**
         *  Final specialisation
         */
        template<class O>
        struct storage_find_impl_type<storage_impl<>, O, void> {
            using type = storage_impl<>;
        };

        /**
         *  storage_t specialization
         */
        template<class... Ts, class O>
        struct storage_find_impl_type<storage_t<Ts...>, O, void>
            : storage_find_impl_type<typename storage_t<Ts...>::impl_type, O> {};

        /**
         *  S - storage_t or storage_impl type
         *  Lookup - mapped data type or CTE label
         */
        template<class S, class Lookup>
        using storage_find_impl_t =
            std::conditional_t<std::is_const<S>::value,
                               std::add_const_t<typename storage_find_impl_type<std::remove_const_t<S>, Lookup>::type>,
                               typename storage_find_impl_type<S, Lookup>::type>;
    }

    /**
      *  S - storage_t type
      *  Lookup - mapped data type or CTE label
      */
    template<class S, class Lookup>
    using storage_pick_impl_t = internal::storage_pick_impl_t<S, Lookup>;

    /**
      *  S - storage_t type
      *  Lookup - mapped data type or CTE label
      */
    template<class S, class Lookup>
    using storage_find_impl_t = internal::storage_find_impl_t<S, Lookup>;
}

// runtime lookup functions
namespace sqlite_orm {
    namespace internal {
        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_pick_impl_t<S, Lookup>& pick_impl(S& strg) {
            return strg;
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_find_impl_t<S, Lookup>& find_impl(S& strg) {
            return strg;
        }
    }
}
