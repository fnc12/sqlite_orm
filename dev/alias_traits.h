#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <type_traits>  //  std::is_base_of, std::is_same, std::remove_const
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
#include <concepts>
#endif
#endif

#include "functional/cxx_type_traits_polyfill.h"
#include "functional/type_traits.h"
#include "vocabulary/traits/grammar_traits_fwd.h"  // Included to specialize traits
#include "vocabulary/traits/structural_traits_fwd.h"  //  is_table_reference

SQLITE_ORM_EXPORT namespace sqlite_orm {

    /** @short Base class for a custom table alias, column alias or expression alias.
     */
    struct alias_tag {};
}

namespace sqlite_orm::internal {
    template<class O>
    struct table_reference;

    template<class RecordSet>
    struct decay_table_ref : std::remove_const<RecordSet> {};
    template<class O>
    struct decay_table_ref<table_reference<O>> : polyfill::type_identity<O> {};
    template<class O>
    struct decay_table_ref<const table_reference<O>> : polyfill::type_identity<O> {};

    template<class RecordSet>
    using decay_table_ref_t = typename decay_table_ref<RecordSet>::type;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<auto recordset>
    using auto_decay_table_ref_t = typename decay_table_ref<decltype(recordset)>::type;
#endif

    //  note: aliases are user-definable, hence the alias traits are defined here, next to `alias_tag`,
    //  rather than in the header of a sqlite_orm node
    template<class T>
    constexpr bool is_alias_v = std::is_base_of<alias_tag, T>::value;

    //  note: the definitions below are written in terms of the variable templates rather than of the `is_*` alias
    //  templates of the traits - msvc 141 fails to resolve an alias template in the initializer of a variable template
    //  [SQLITE_ORM_BROKEN_ALIAS_TEMPLATE_DEPENDENT_NTTP_EXPR];
    //  and they name their template parameter like the declarations in `grammar_traits_fwd.h` do - msvc 141 resolves
    //  the initializer of a redeclared variable template with the parameter names of the first declaration

    /** @short Alias of a column in a record set, see `orm_column_alias`.
     */
    template<class T>
    constexpr bool is_column_alias_v = is_alias_v<T> && !polyfill::is_detected_v<type_t, T>;

    /** @short Alias of any type of record set, see `orm_recordset_alias`.
     */
    template<class T>
    constexpr bool is_recordset_alias_v = is_alias_v<T> && polyfill::is_detected_v<type_t, T>;

    /** @short Alias of a concrete table, see `orm_table_alias`.
     */
    template<class T>
    constexpr bool is_table_alias_v =
        is_recordset_alias_v<T> && !std::is_same<polyfill::detected_t<type_t, T>, std::remove_const_t<T>>::value;

    /** @short Moniker of a CTE, see `orm_cte_moniker`.
     */
    template<class T>
    constexpr bool is_cte_moniker_v =
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
        is_recordset_alias_v<T> && std::is_same<polyfill::detected_t<type_t, T>, std::remove_const_t<T>>::value;
#else
        false;
#endif

    /** @short Referring to a recordset.
     */
    template<class T>
    inline constexpr bool is_referring_to_recordset_v = is_table_reference_v<T> || is_recordset_alias_v<T>;

    template<class T>
    using is_referring_to_recordset = std::bool_constant<is_referring_to_recordset_v<T>>;
}

SQLITE_ORM_EXPORT namespace sqlite_orm {
#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
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

    /** @short Specifies that a type is a reference of a concrete table, especially of a derived class.
     *  
     *  A concrete table reference has the following traits:
     *  - specialization of `table_reference`, whose `type` typename references a mapped object.
     */
    template<class O>
    concept orm_table_reference = polyfill::is_specialization_of_v<std::remove_const_t<O>, internal::table_reference>;

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
