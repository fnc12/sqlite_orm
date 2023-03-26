#pragma once

#include <string>  //  std::string
#include <utility>  //  std::move

#include "functional/cxx_core_features.h"
#include "conditions.h"
#include "alias_traits.h"

namespace sqlite_orm {
    namespace internal {
        /**
         *  This class is used to store explicit mapped type T and its column descriptor (member pointer/getter/setter).
         *  Is useful when mapped type is derived from other type and base class has members mapped to a storage.
         */
        template<class T, class F>
        struct column_pointer {
            using self = column_pointer<T, F>;
            using type = T;
            using field_type = F;

            field_type field;

            template<class R>
            is_equal_t<self, R> operator==(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            is_not_equal_t<self, R> operator!=(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            lesser_than_t<self, R> operator<(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            lesser_or_equal_t<self, R> operator<=(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            greater_than_t<self, R> operator>(R rhs) const {
                return {*this, std::move(rhs)};
            }

            template<class R>
            greater_or_equal_t<self, R> operator>=(R rhs) const {
                return {*this, std::move(rhs)};
            }
        };

        template<class T>
        SQLITE_ORM_INLINE_VAR constexpr bool is_column_pointer_v = polyfill::is_specialization_of_v<T, column_pointer>;

        template<class T>
        using is_column_pointer = polyfill::bool_constant<is_column_pointer_v<T>>;

        template<class O, class SFINAE = void>
        struct column_pointer_builder {
            using mapped_type = O;

            /**
             *  Explicitly refer to a column, used in contexts
             *  where the automatic object mapping deduction needs to be overridden.
             *
             *  Example:
             *  struct MyType : BaseType { ... };
             *  storage.select(column<MyType>(&BaseType::id));
             */
            template<class F>
            column_pointer<mapped_type, F> operator()(F field) const {
                return {std::move(field)};
            }
        };

#ifdef SQLITE_ORM_WITH_CTE
        template<class A>
        struct alias_holder;

        template<class A>
        struct column_pointer_builder<A, match_if<is_alias, A>> {
            static_assert(is_cte_moniker_v<A>, "`A' must be a mapped table alias");

            using mapped_type = A;

            /**
             *  Explicitly refer to a column alias mapped into a CTE or subquery.
             *
             *  Example:
             *  struct Object { ... };
             *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(&Object::id)));
             */
            template<class F, satisfies_not<is_column_alias, F> = true>
            constexpr column_pointer<mapped_type, F> operator()(F field) const {
                return {std::move(field)};
            }

            /**
             *  Explicitly refer to a column alias mapped into a CTE or subquery.
             *
             *  Example:
             *  struct Object { ... };
             *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>(1_colalias)));
             *  storage.with(cte<cte_1>()(select(as<colalias_a>(&Object::id))), select(column<cte_1>(colalias_a{})));
             *  storage.with(cte<cte_1>(colalias_a{})(select(&Object::id)), select(column<cte_1>(colalias_a{})));
             */
            template<class ColAlias, satisfies<is_column_alias, ColAlias> = true>
            constexpr column_pointer<mapped_type, alias_holder<ColAlias>> operator()(const ColAlias&) const {
                return {{}};
            }

            /**
             *  Explicitly refer to a column alias mapped into a CTE or subquery.
             *
             *  Example:
             *  struct Object { ... };
             *  storage.with(cte<cte_1>()(select(as<colalias_a>(&Object::id))), select(column<cte_1>(get<colalias_a>())));
             */
            template<class ColAlias, satisfies<is_column_alias, ColAlias> = true>
            constexpr column_pointer<mapped_type, alias_holder<ColAlias>>
            operator()(const alias_holder<ColAlias>&) const {
                return {{}};
            }
        };
#endif

    }

    /**
     *  Explicit column references.
     */
    template<class Lookup>
    SQLITE_ORM_INLINE_VAR constexpr internal::column_pointer_builder<Lookup> column{};

    /**
     *  Explicitly refer to a column mapped into a CTE or subquery.
     *
     *  Example:
     *  struct Object { ... };
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>->*&Object::id));
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(column<cte_1>->*1_colalias));
     */
    template<class A, class F, internal::satisfies<internal::is_cte_moniker, A> = true>
    constexpr auto operator->*(const A& /*cteAlias*/, F field) {
        return column<A>(std::move(field));
    }

    /**
     *  Explicitly refer to an aliased column mapped into a CTE or subquery.
     *
     *  Use it like this:
     *  struct Object { ... };
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(1_ctealias->*&Object::id));
     *  storage.with(cte<cte_1>()(select(&Object::id)), select(1_ctealias->*1_colalias));
     */
    template<class A, class F, internal::satisfies<internal::is_cte_moniker, A> = true>
    constexpr auto operator->*(const internal::column_pointer_builder<A>&, F field) {
        return column<A>(std::move(field));
    }
}