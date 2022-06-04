#pragma once

#include <type_traits>
#include <tuple>

#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class... Ts>
        struct storage_t;

        template<class... Ts>
        using schema_objects = std::tuple<Ts...>;

        template<class T>
        struct is_storage : std::false_type {};

        template<class... Ts>
        struct is_storage<storage_t<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage<const storage_t<Ts...>> : std::true_type {};

        template<class T>
        struct is_storage_impl : std::false_type {};

        template<class... Ts>
        struct is_storage_impl<schema_objects<Ts...>> : std::true_type {};
        template<class... Ts>
        struct is_storage_impl<const schema_objects<Ts...>> : std::true_type {};

        /**
         *  std::true_type if given object is mapped, std::false_type otherwise.
         * 
         *  Note: unlike table_t<>, index_t<>::object_type is always void.
         */
        template<typename SO, typename O>
        struct object_type_matches : polyfill::conjunction<polyfill::negation<std::is_void<object_type_t<SO>>>,
                                                           std::is_same<O, object_type_t<SO>>> {};

        /**
         *  std::true_type if given lookup type (object) is mapped, std::false_type otherwise.
         */
        template<typename SO, typename Lookup>
        struct lookup_type_matches : polyfill::disjunction<object_type_matches<SO, Lookup>> {};
    }

    // pick/lookup metafunctions
    namespace internal {

        /**
         *  S - schema_objects type
         *  O - mapped data type
         */
        template<class O, class... SOs>
        struct storage_pick_table : std::enable_if<lookup_type_matches<SOs, O>::value, SOs>... {};

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class O, class... SOs>
        struct storage_pick_table<O, schema_objects<SOs...>> : storage_pick_table<O, SOs...> {};
#else
        template<class O, class... SOs>
        struct storage_pick_table<O, std::tuple<SOs...>> : storage_pick_table<O, SOs...> {};
#endif

        /**
         *  S - schema_objects type, possibly const-qualified
         *  Lookup - 'table' type, mapped data type
         */
        template<class Lookup, class S>
        using storage_pick_table_t = typename storage_pick_table<Lookup, std::remove_const_t<S>>::type;

        /**
         *  S - schema_objects type
         *  O - mapped data type
         */
        template<class O, class... SOs>
        struct storage_find_table : polyfill::detected_or<polyfill::nonesuch, storage_pick_table_t, O, SOs...> {};

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class O, class... SOs>
        struct storage_find_table<O, schema_objects<SOs...>> : storage_find_table<O, SOs...> {};
#else
        template<class O, class... SOs>
        struct storage_find_table<O, std::tuple<SOs...>> : storage_find_table<O, SOs...> {};
#endif

        /**
         *  S - schema_objects type, possibly const-qualified
         *  Lookup - 'table' type, mapped data type
         */
        template<class Lookup, class S>
        using storage_find_table_t = typename storage_find_table<Lookup, std::remove_const_t<S>>::type;

        template<class S, class O, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v = false;
        template<class S, class O>
        SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v<S, O, polyfill::void_t<storage_pick_table_t<O, S>>> = true;

        template<class S, class O>
        using is_mapped = polyfill::bool_constant<is_mapped_v<S, O>>;
    }
}

// runtime lookup functions
namespace sqlite_orm {
    namespace internal {
        /**
         *  Given a storage implementation pack, pick the specific schema object for the given lookup type.
         * 
         *  Note: This function requires Lookup to be mapped, otherwise it is removed from the overload resolution set.
         */
        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        auto& pick_table(S& impl) {
            using table_type = storage_pick_table_t<Lookup, S>;
            return std::get<table_type>(impl);
        }
    }
}
