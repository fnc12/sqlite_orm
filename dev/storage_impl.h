#pragma once

#include <string>  //  std::string

#include "functional/cxx_universal.h"  //  ::nullptr_t
#include "functional/static_magic.h"
#include "tuple_helper/tuple_traits.h"
#include "tuple_helper/tuple_filter.h"
#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "cte_types.h"
#include "storage_lookup.h"

// interface functions
namespace sqlite_orm {
    namespace internal {

        template<class DBOs>
        using tables_index_sequence = filter_tuple_sequence_t<DBOs, is_table>;

        template<class DBOs, satisfies<is_db_objects, DBOs> = true>
        int foreign_keys_count(const DBOs& dbObjects) {
            int res = 0;
            iterate_tuple<true>(dbObjects, tables_index_sequence<DBOs>{}, [&res](const auto& table) {
                res += table.template count_of<is_foreign_key>();
            });
            return res;
        }

        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs>>
        decltype(auto) lookup_table_name(const DBOs& dbObjects) {
            return static_if<is_mapped_v<DBOs, Lookup>>(
                [](const auto& dbObjects) -> const std::string& {
                    return pick_table<Lookup>(dbObjects).name;
                },
                empty_callable<std::string>())(dbObjects);
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        const std::string* find_column_name(const DBOs& dbObjects, F O::*field) {
            return pick_table<O>(dbObjects).find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         *  2. by moniker and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        constexpr decltype(auto) materialize_column_pointer(const DBOs&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

#ifdef SQLITE_ORM_WITH_CTE
        /**
         *  Materialize column pointer:
         *  3. by moniker and alias_holder<>.
         *  
         *  internal note: there's an overload for `find_column_name()` that avoids going through `table_t<>::find_column_name()`
         */
        template<class Moniker, class ColAlias, class DBOs, satisfies<is_db_objects, DBOs> = true>
        constexpr decltype(auto) materialize_column_pointer(const DBOs&,
                                                            const column_pointer<Moniker, alias_holder<ColAlias>>&) {
            using table_type = storage_pick_table_t<Moniker, DBOs>;
            using cte_mapper_type = cte_mapper_type_t<table_type>;

            // filter all column references [`alias_holder<>`]
            using alias_types_tuple =
                transform_tuple_t<typename cte_mapper_type::final_colrefs_tuple, alias_holder_type_or_none_t>;

            // lookup index in alias_types_tuple by Alias
            constexpr auto ColIdx = tuple_index_of_v<ColAlias, alias_types_tuple>;
            static_assert(ColIdx != -1, "No such column mapped into the CTE.");

            return &aliased_field<ColAlias, std::tuple_element_t<ColIdx, typename cte_mapper_type::fields_type>>::field;
        }
#endif

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         *  2. by moniker and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        const std::string* find_column_name(const DBOs& dbObjects, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(dbObjects, cp);
            return pick_table<O>(dbObjects).find_column_name(field);
        }

#ifdef SQLITE_ORM_WITH_CTE
        /**
         *  Find column name by:
         *  3. by moniker and alias_holder<>.
         */
        template<class Moniker, class ColAlias, class DBOs, satisfies<is_db_objects, DBOs> = true>
        constexpr decltype(auto) find_column_name(const DBOs& dboObjects,
                                                  const column_pointer<Moniker, alias_holder<ColAlias>>&) {
            using table_type = storage_pick_table_t<Moniker, DBOs>;
            using cte_mapper_type = cte_mapper_type_t<table_type>;
            using elements_t = typename table_type::elements_type;
            using column_idxs = filter_tuple_sequence_t<elements_t, is_column>;

            // note: even though the columns contain the [`aliased_field<>::*`] we perform the lookup using the column references.
            // filter all column references [`alias_holder<>`]
            using alias_types_tuple =
                transform_tuple_t<typename cte_mapper_type::final_colrefs_tuple, alias_holder_type_or_none_t>;

            // lookup index of ColAlias in alias_types_tuple
            constexpr auto I = tuple_index_of_v<ColAlias, alias_types_tuple>;
            static_assert(I != -1, "No such column mapped into the CTE.");

            // note: we could "materialize" the alias to an `aliased_field<>::*` and use the regular `table_t<>::find_column_name()` mechanism;
            //       however we have the column index already.
            // lookup column in table_t<>'s elements
            constexpr size_t ColIdx = index_sequence_value(I, column_idxs{});
            auto& table = pick_table<Moniker>(dboObjects);
            return &std::get<ColIdx>(table.elements).name;
        }
#endif
    }
}
