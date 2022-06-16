#pragma once

#include <type_traits>  //  std::true_type, std::false_type, std::remove_const, std::enable_if
#include <tuple>

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"

namespace sqlite_orm {
    namespace internal {

        template<class... DBO>
        struct storage_t;

        template<class... DBO>
        using db_objects_tuple = std::tuple<DBO...>;

        template<class T>
        struct is_storage : std::false_type {};

        template<class... DBO>
        struct is_storage<storage_t<DBO...>> : std::true_type {};
        template<class... DBO>
        struct is_storage<const storage_t<DBO...>> : std::true_type {};

        template<class T>
        struct is_db_objects : std::false_type {};

        template<class... DBO>
        struct is_db_objects<db_objects_tuple<DBO...>> : std::true_type {};
        template<class... DBO>
        struct is_db_objects<const db_objects_tuple<DBO...>> : std::true_type {};

        /**
         *  std::true_type if given object is mapped, std::false_type otherwise.
         * 
         *  Note: unlike table_t<>, index_t<>::object_type and trigger_t<>::object_type is always void.
         */
        template<typename DBO, typename Lookup>
        struct object_type_matches : polyfill::conjunction<polyfill::negation<std::is_void<object_type_t<DBO>>>,
                                                           std::is_same<Lookup, object_type_t<DBO>>> {};

        /**
         *  std::true_type if given lookup type (object) is mapped, std::false_type otherwise.
         */
        template<typename DBO, typename Lookup>
        struct lookup_type_matches : polyfill::disjunction<object_type_matches<DBO, Lookup>> {};
    }

    // pick/lookup metafunctions
    namespace internal {

        /**
         *  SFINAE friendly facility to pick a table definition (`table_t`) from a tuple of database objects.
         *  
         *  DBOs - db_objects_tuple type
         *  Lookup - mapped data type
         */
        template<class Lookup, class... DBO>
        struct storage_pick_table : std::enable_if<lookup_type_matches<DBO, Lookup>::value, DBO>... {};

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class Lookup, class... DBO>
        struct storage_pick_table<Lookup, db_objects_tuple<DBO...>> : storage_pick_table<Lookup, DBO...> {};
#else
        template<class Lookup, class... DBO>
        struct storage_pick_table<Lookup, std::tuple<DBO...>> : storage_pick_table<Lookup, DBO...> {};
#endif

        /**
         *  SFINAE friendly facility to pick a table definition (`table_t`) from a tuple of database objects.
         *
         *  DBOs - db_objects_tuple type, possibly const-qualified
         *  Lookup - 'table' type, mapped data type
         */
        template<class Lookup, class DBOs>
        using storage_pick_table_t = typename storage_pick_table<Lookup, std::remove_const_t<DBOs>>::type;

        /**
         *  Find a table definition (`table_t`) from a tuple of database objects;
         *  `std::nonesuch` if not found.
         *
         *  DBOs - db_objects_tuple type
         *  Lookup - mapped data type
         */
        template<class Lookup, class... DBO>
        struct storage_find_table : polyfill::detected_or<polyfill::nonesuch, storage_pick_table_t, Lookup, DBO...> {};

#ifndef SQLITE_ORM_BROKEN_VARIADIC_PACK_EXPANSION
        template<class Lookup, class... DBO>
        struct storage_find_table<Lookup, db_objects_tuple<DBO...>> : storage_find_table<Lookup, DBO...> {};
#else
        template<class Lookup, class... DBO>
        struct storage_find_table<Lookup, std::tuple<DBO...>> : storage_find_table<Lookup, DBO...> {};
#endif

        /**
         *  Find a table definition (`table_t`) from a tuple of database objects;
         *  `std::nonesuch` if not found.
         *
         *  DBOs - db_objects_tuple type, possibly const-qualified
         *  Lookup - mapped data type
         */
        template<class Lookup, class DBOs>
        using storage_find_table_t = typename storage_find_table<Lookup, std::remove_const_t<DBOs>>::type;

        template<class DBOs, class Lookup, class SFINAE = void>
        SQLITE_ORM_INLINE_VAR constexpr bool is_mapped_v = false;
        template<class DBOs, class Lookup>
        SQLITE_ORM_INLINE_VAR constexpr bool
            is_mapped_v<DBOs, Lookup, polyfill::void_t<storage_pick_table_t<Lookup, DBOs>>> = true;

        template<class DBOs, class Lookup>
        using is_mapped = polyfill::bool_constant<is_mapped_v<DBOs, Lookup>>;
    }
}

// runtime lookup functions
namespace sqlite_orm {
    namespace internal {
        /**
         *  Pick the table definition for the specified lookup type from the given tuple of schema objects.
         * 
         *  Note: This function requires Lookup to be mapped, otherwise it is removed from the overload resolution set.
         */
        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
        auto& pick_table(DBOs& dbObjects) {
            using table_type = storage_pick_table_t<Lookup, DBOs>;
            return std::get<table_type>(dbObjects);
        }
    }
}
