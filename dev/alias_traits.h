#pragma once

#include <type_traits>  //  std::remove_const, std::is_base_of, std::is_same

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    /**
     *  Base class for a custom table alias, column alias or expression alias.
     *  For more information please look through self_join.cpp example
     */
    struct alias_tag {};

    namespace internal {

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_alias_v = std::is_base_of<alias_tag, A>::value;

        template<class A>
        using is_alias = polyfill::bool_constant<is_alias_v<A>>;

        /** Alias for a column in a record set.
         *
         *  A column alias has the following traits:
         *  - is derived from `alias_tag`
         *
         *  @note: Currently, there is no distinguishing property of a column alias other than that it is derived from "alias_tag".
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_alias_v = is_alias_v<A>;

        template<class A>
        using is_column_alias = is_alias<A>;

        /** Alias for any type of record set.
         *
         *  A record set alias has the following traits:
         *  - is derived from `alias_tag`.
         *  - has a `type` typename, which refers to a mapped object.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_recordset_alias_v =
            polyfill::conjunction_v<is_alias<A>, polyfill::is_detected<type_t, A>>;

        template<class A>
        using is_recordset_alias = polyfill::bool_constant<is_recordset_alias_v<A>>;

        /** Alias for a concrete table.
         *  
         *  A concrete table alias has the following traits:
         *  - is derived from `alias_tag`.
         *  - has a `type` typename, which refers to another mapped object (i.e. doesn't refer to itself).
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_table_alias_v = polyfill::conjunction_v<
            is_recordset_alias<A>,
            polyfill::negation<std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>>;

        template<class A>
        using is_table_alias = polyfill::bool_constant<is_table_alias_v<A>>;
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    template<class T>
    concept orm_column_alias = internal::is_column_alias_v<T>;

    template<class T>
    concept orm_recordset_alias = internal::is_recordset_alias_v<T>;

    template<class T>
    concept orm_table_alias = internal::is_table_alias_v<T>;
#endif
}
