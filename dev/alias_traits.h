#pragma once

#ifndef _IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::is_same
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>
#endif
#endif

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "table_reference.h"

namespace sqlite_orm {

    /** @short Base class for a custom table alias, column alias or expression alias.
     */
    struct alias_tag {};

    namespace internal {

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_alias_v = std::is_base_of<alias_tag, A>::value;

        template<class A>
        struct is_alias : polyfill::bool_constant<is_alias_v<A>> {};

        /** @short Alias of a column in a record set, see `orm_column_alias`.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_alias_v =
            polyfill::conjunction<is_alias<A>, polyfill::negation<polyfill::is_detected<type_t, A>>>::value;

        template<class A>
        struct is_column_alias : is_alias<A> {};

        /** @short Alias of any type of record set, see `orm_recordset_alias`.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_recordset_alias_v =
            polyfill::conjunction<is_alias<A>, polyfill::is_detected<type_t, A>>::value;

        template<class A>
        struct is_recordset_alias : polyfill::bool_constant<is_recordset_alias_v<A>> {};

        /** @short Alias of a concrete table, see `orm_table_alias`.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_table_alias_v = polyfill::conjunction<
            is_recordset_alias<A>,
            polyfill::negation<std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>>::value;

        template<class A>
        struct is_table_alias : polyfill::bool_constant<is_table_alias_v<A>> {};

        /** @short Moniker of a CTE, see `orm_cte_moniker`.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_cte_moniker_v =
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
            polyfill::conjunction_v<is_recordset_alias<A>,
                                    std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>;
#else
            false;
#endif

        template<class A>
        using is_cte_moniker = polyfill::bool_constant<is_cte_moniker_v<A>>;
    }
}

_EXPORT_SQLITE_ORM namespace sqlite_orm {
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<class A>
    concept orm_alias = std::derived_from<A, alias_tag>;

    /** @short Specifies that a type is an alias of a column in a record set.
     *
     *  A column alias has the following traits:
     *  - is derived from `alias_tag`
     *  - must not have a nested `type` typename
     */
    template<class A>
    concept orm_column_alias = (orm_alias<A> && !orm_names_type<A>);

    /** @short Specifies that a type is an alias of any type of record set.
     *
     *  A record set alias has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a nested `type` typename, which refers to a mapped object.
     */
    template<class A>
    concept orm_recordset_alias = (orm_alias<A> && orm_names_type<A>);

    /** @short Specifies that a type is an alias of a concrete table.
     *
     *  A concrete table alias has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a `type` typename, which refers to another mapped object (i.e. doesn't refer to itself).
     */
    template<class A>
    concept orm_table_alias = (orm_recordset_alias<A> && !std::same_as<typename A::type, std::remove_const_t<A>>);

    /** @short Moniker of a CTE.
     *
     *  A CTE moniker has the following traits:
     *  - is derived from `alias_tag`.
     *  - has a `type` typename, which refers to itself.
     */
    template<class A>
    concept orm_cte_moniker = (orm_recordset_alias<A> && std::same_as<typename A::type, std::remove_const_t<A>>);

    /** @short Specifies that a type refers to a mapped table (possibly aliased).
     */
    template<class T>
    concept orm_refers_to_table = (orm_table_reference<T> || orm_table_alias<T>);

    /** @short Specifies that a type refers to a recordset.
     */
    template<class T>
    concept orm_refers_to_recordset = (orm_table_reference<T> || orm_recordset_alias<T>);

    /** @short Specifies that a type is a mapped recordset (table reference).
     */
    template<class T>
    concept orm_mapped_recordset = (orm_table_reference<T> || orm_cte_moniker<T>);
#endif
}
