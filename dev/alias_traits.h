#pragma once

#include <type_traits>  //  std::remove_const, std::is_base_of, std::is_same

#include "functional/cxx_universal.h"
#include "functional/cxx_type_traits_polyfill.h"

namespace sqlite_orm {

    /**
     *  This is base class for every class which is used as a custom table alias, column alias or expression alias.
     *  For more information please look through self_join.cpp example
     */
    struct alias_tag {};

    namespace internal {

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_alias_v = std::is_base_of<alias_tag, A>::value;

        template<class A>
        using is_alias = polyfill::bool_constant<is_alias_v<A>>;

        /*
         *  Note: Currently, there is no distinguishing property of a column alias other than that it is derived from "alias_tag".
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_alias_v = is_alias_v<A>;

        template<class A>
        using is_column_alias = is_alias<A>;

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_any_table_alias_v =
            polyfill::conjunction_v<is_alias<A>, polyfill::is_detected<type_t, A>>;

        template<class A>
        using is_any_table_alias = polyfill::bool_constant<is_any_table_alias_v<A>>;

        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_table_alias_v = polyfill::conjunction_v<
            std::is_base_of<alias_tag, A>,
            polyfill::negation<std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>>;

        template<class A>
        using is_table_alias = polyfill::bool_constant<is_table_alias_v<A>>;

        /**
         *  A CTE alias is a specialization of a table alias, which is both, a storage lookup type (mapping type) and an alias.
         */
        template<class A>
        SQLITE_ORM_INLINE_VAR constexpr bool is_cte_alias_v =
#ifdef SQLITE_ORM_WITH_CTE
            polyfill::conjunction_v<std::is_base_of<alias_tag, A>,
                                    std::is_same<polyfill::detected_t<type_t, A>, std::remove_const_t<A>>>;
#else
            false;
#endif

        template<class A>
        using is_cte_alias = polyfill::bool_constant<is_cte_alias_v<A>>;

        template<class T>
        struct alias_holder {
            using type = T;

            alias_holder() = default;
            // CTE feature needs it to implicitly convert a column alias to an alias_holder; see `cte()` factory function
            alias_holder(const T&) noexcept {}
        };
    }
}
