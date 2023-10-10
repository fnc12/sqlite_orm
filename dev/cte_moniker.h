#pragma once

#ifdef SQLITE_ORM_WITH_CTE
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
#include <concepts>
#include <utility>  //  std::make_index_sequence
#endif
#include <type_traits>  //  std::enable_if, std::is_member_pointer, std::is_same, std::is_convertible
#include <tuple>  //  std::ignore
#include <string>
#endif

#include "functional/cxx_universal.h"
#include "alias.h"

#ifdef SQLITE_ORM_WITH_CTE
namespace sqlite_orm {

    namespace internal {
        /** 
         *  A special record set alias that is both, a storage lookup type (mapping type) and an alias.
         */
        template<char A, char... X>
        struct cte_moniker
            : recordset_alias<
                  cte_moniker<A, X...> /* refer to self, since a moniker is both, an alias and a mapped type */,
                  A,
                  X...> {
            /** 
             *  Introduce the construction of a common table expression using this moniker.
             *  
             *  The list of explicit columns is optional;
             *  if provided the number of columns must match the number of columns of the subselect.
             *  The column names will be merged with the subselect:
             *  1. column names of subselect
             *  2. explicit columns
             *  3. fill in empty column names with column index
             *  
             *  Example:
             *  1_ctealias()(select(&Object::id));
             *  1_ctealias(&Object::name)(select("object"));
             *  
             *  @return A `cte_builder` instance.
             *  @note (internal): Defined in select_constraints.h in order to keep this member function in the same place as the named factory function `cte()`,
             *  and to keep the actual creation of the builder in one place.
             */
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
            template<class... ExplicitCols>
                requires((is_column_alias_v<ExplicitCols> || std::is_member_pointer_v<ExplicitCols> ||
                          std::same_as<ExplicitCols, std::remove_cvref_t<decltype(std::ignore)>> ||
                          std::convertible_to<ExplicitCols, std::string>) &&
                         ...)
            auto operator()(ExplicitCols... explicitColumns) const;
#else
            template<class... ExplicitCols,
                     std::enable_if_t<polyfill::conjunction_v<polyfill::disjunction<
                                          is_column_alias<ExplicitCols>,
                                          std::is_member_pointer<ExplicitCols>,
                                          std::is_same<ExplicitCols, polyfill::remove_cvref_t<decltype(std::ignore)>>,
                                          std::is_convertible<ExplicitCols, std::string>>...>,
                                      bool> = true>
            auto operator()(ExplicitCols... explicitColumns) const;
#endif
        };
    }

    /**
     *  cte_moniker<'n'> from a numeric literal.
     *  E.g. 1_ctealias, 2_ctealias
     */
    template<char... Chars>
    [[nodiscard]] SQLITE_ORM_CONSTEVAL auto operator"" _ctealias() {
        return internal::cte_moniker<Chars...>{};
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    /**
     *  cte_moniker<'1'[, ...]> from a string literal.
     *  E.g. "1"_cte, "2"_cte
     */
    template<internal::string_identifier_template t>
    [[nodiscard]] consteval auto operator"" _cte() {
        return internal::to_alias<internal::cte_moniker, t>(std::make_index_sequence<t.size()>{});
    }
#endif
}
#endif
