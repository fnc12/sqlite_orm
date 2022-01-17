#pragma once

#include <type_traits>

#include "cxx_polyfill.h"
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
         *  A data type's CTE alias, otherwise T itself is used as a CTE alias.
         */
        template<typename T>
        using detected_cte_label_t = polyfill::detected_or_t<T, cte_label_type_t, T>;

        /**
         *  std::true_type if given object is mapped, std::false_type otherwise
         */
        template<typename S, typename O>
        using object_type_matches = typename polyfill::conjunction<std::is_void<storage_cte_label_type_t<S>>,
                                                                   std::is_same<O, storage_object_type_t<S>>>::type;

        /**
         *  std::true_type if object is mapped via given (CTE) label, std::false_type otherwise
         */
        template<typename S, typename Label>
        using cte_label_type_matches = typename polyfill::conjunction<
            std::negation<std::is_void<storage_cte_label_type_t<S>>>,
            std::is_same<detected_cte_label_t<Label>, storage_cte_label_type_t<S>>>::type;

        template<typename S, typename Lookup>
        using lookup_type_matches =
            typename polyfill::disjunction<object_type_matches<S, Lookup>, cte_label_type_matches<S, Lookup>>::type;
    }

    // pick/lookup metafunctions
    namespace internal {

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
    }

    namespace internal {

        /**
         *  S - storage_t or storage_impl type
         *  Lookup - mapped data type or CTE label
         */
        template<class S, class Lookup>
        using storage_pick_impl_t =
            std::conditional_t<std::is_const<S>::value,
                               const typename storage_pick_impl_type<std::remove_const_t<S>, Lookup>::type,
                               // note: need to remove const on this branch, too, due to SFINAE
                               typename storage_pick_impl_type<std::remove_const_t<S>, Lookup>::type>;

        /**
         *  S - storage_t or storage_impl type
         *  Lookup - mapped data type or CTE label
         */
        template<class S, class Lookup>
        using storage_find_impl_t =
            std::conditional_t<std::is_const<S>::value,
                               const typename storage_find_impl_type<std::remove_const_t<S>, Lookup>::type,
                               // note: need to remove const on this branch, too, due to SFINAE
                               typename storage_find_impl_type<std::remove_const_t<S>, Lookup>::type>;
    }
}

// runtime lookup functions
namespace sqlite_orm {
    namespace internal {
        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_pick_impl_t<S, Lookup>& pick_impl(S& impl) {
            return impl;
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        storage_find_impl_t<S, Lookup>& find_impl(S& impl) {
            return impl;
        }

        template<class Lookup, class S, satisfies<is_storage, S> = true>
        storage_pick_impl_t<const S, Lookup>& pick_const_impl(S& storage) {
            return obtain_const_impl(storage);
        }
    }
}
