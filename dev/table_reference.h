#pragma once

#ifndef _IMPORT_STD_MODULE
#include <type_traits>  //  std::remove_const, std::type_identity
#endif

#include "functional/cxx_type_traits_polyfill.h"

_EXPORT_SQLITE_ORM namespace sqlite_orm {
    namespace internal {
        /*
         *  Identity wrapper around a mapped object, facilitating uniform column pointer expressions.
         */
        template<class O>
        struct table_reference : polyfill::type_identity<O> {};

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

        template<class R>
        SQLITE_ORM_INLINE_VAR constexpr bool is_table_reference_v =
            polyfill::is_specialization_of_v<std::remove_const_t<R>, table_reference>;

        template<class R>
        struct is_table_reference : polyfill::bool_constant<is_table_reference_v<R>> {};
    }

#ifdef SQLITE_ORM_CPP20_CONCEPTS_SUPPORTED
    /** @short Specifies that a type is a reference of a concrete table, especially of a derived class.
     *
     *  A concrete table reference has the following traits:
     *  - specialization of `table_reference`, whose `type` typename references a mapped object.
     */
    template<class R>
    concept orm_table_reference = polyfill::is_specialization_of_v<std::remove_const_t<R>, internal::table_reference>;
#endif
}
