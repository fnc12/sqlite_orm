#pragma once

#include <type_traits>  //  std::enable_if, std::remove_const
#include <utility>  // std::move

#include "functional/cxx_core_features.h"
#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "alias_traits.h"
#include "tags.h"

namespace sqlite_orm {
    namespace internal {
        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer {
            using type = T;
            using field_type = F;

            field_type field;
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_pointer_v = polyfill::is_specialization_of_v<T, column_pointer>;

        template<class T>
        using is_column_pointer = polyfill::bool_constant<is_column_pointer_v<T>>;

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_operator_argument_v<T, std::enable_if_t<is_column_pointer_v<T>>> = true;

#ifdef SQLITE_ORM_WITH_CTE
        template<class A>
        struct alias_holder;
#endif
    }

    /**
     *  Explicitly refer to a column, used in contexts
     *  where the automatic object mapping deduction needs to be overridden.
     *
     *  Example:
     *  struct BaseType : { int64 id; };
     *  struct MyType : BaseType { ... };
     *  storage.select(column<MyType>(&BaseType::id));
     */
    template<class Object, class F, internal::satisfies_not<internal::is_recordset_alias, Object> = true>
    constexpr internal::column_pointer<Object, F> column(F field) {
        return {std::move(field)};
    }

#ifdef SQLITE_ORM_WITH_CTE
    /**
     *  Explicitly refer to a column alias mapped into a CTE or subquery.
     *
     *  Example:
     *  struct Object { ... };
     *  using cte_1 = decltype(1_ctealias);
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(&Object::id)));
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(1_colalias)));
     *  storage.with(cte<cte_1>()(select(as<colalias_a>(&Object::id))), select(column<cte_1>(colalias_a{})));
     *  storage.with(cte<cte_1>(colalias_a{})(select(&Object::id)), select(column<cte_1>(colalias_a{})));
     *  storage.with(cte<cte_1>()(select(as<colalias_a>(&Object::id))), select(column<cte_1>(get<colalias_a>())));
     */
    template<class Moniker, class F, internal::satisfies<internal::is_recordset_alias, Moniker> = true>
    constexpr auto column(F field) {
        using namespace ::sqlite_orm::internal;

        static_assert(is_cte_moniker_v<Moniker>, "`Moniker' must be a CTE moniker");

        if constexpr(polyfill::is_specialization_of_v<F, alias_holder>) {
            static_assert(is_column_alias_v<type_t<F>>);
            return column_pointer<Moniker, F>{{}};
        } else if constexpr(is_column_alias_v<F>) {
            return column_pointer<Moniker, alias_holder<F>>{{}};
        } else {
            return column_pointer<Moniker, F>{std::move(field)};
        }
        (void)field;
    }

#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  Explicitly refer to a column mapped into a CTE or subquery.
     *
     *  Example:
     *  struct Object { ... };
     *  storage.with(cte<"z"_cte>()(select(&Object::id)), select(column<"z"_cte>(&Object::id)));
     *  storage.with(cte<"z"_cte>()(select(&Object::id)), select(column<"z"_cte>(1_colalias)));
     */
    template<orm_cte_moniker auto moniker, class F>
    constexpr auto column([[maybe_unused]] F field) {
        using namespace ::sqlite_orm::internal;
        using Moniker = std::remove_const_t<decltype(moniker)>;

        if constexpr(polyfill::is_specialization_of_v<F, alias_holder>) {
            static_assert(is_column_alias_v<type_t<F>>);
            return column_pointer<Moniker, F>{{}};
        } else if constexpr(is_column_alias_v<F>) {
            return column_pointer<Moniker, alias_holder<F>>{{}};
        } else {
            return column_pointer<Moniker, F>{std::move(field)};
        }
    }
#endif

    /**
     *  Explicitly refer to a column mapped into a CTE or subquery.
     *
     *  Example:
     *  struct Object { ... };
     *  using cte_1 = decltype(1_ctealias);
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(1_ctealias->*&Object::id));
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(1_ctealias->*1_colalias));
     */
    template<class Moniker, class F, internal::satisfies<internal::is_cte_moniker, Moniker> = true>
    constexpr auto operator->*(const Moniker& /*moniker*/, F field) {
        using namespace ::sqlite_orm::internal;

        if constexpr(polyfill::is_specialization_of_v<F, alias_holder>) {
            static_assert(is_column_alias_v<type_t<F>>);
            return column_pointer<Moniker, F>{{}};
        } else if constexpr(is_column_alias_v<F>) {
            return column_pointer<Moniker, alias_holder<F>>{{}};
        } else {
            return column_pointer<Moniker, F>{std::move(field)};
        }
        (void)field;
    }
#endif
}