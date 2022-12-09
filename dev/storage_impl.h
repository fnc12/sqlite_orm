#pragma once

#include <tuple>  //  std::tuple_size
#include <typeindex>  //  std::type_index
#include <string>  //  std::string
#include <type_traits>  //  std::is_same, std::decay, std::make_index_sequence, std::index_sequence

#include "functional/cxx_universal.h"
#include "functional/static_magic.h"
#include "tuple_helper/tuple_iteration.h"
#include "type_traits.h"
#include "select_constraints.h"
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
                res += table.foreign_keys_count();
            });
            return res;
        }

        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
        auto lookup_table(const DBOs& dbObjects) {
            return static_if<is_mapped_v<DBOs, Lookup>>(
                [](const auto& dbObjects) {
                    return &pick_table<Lookup>(dbObjects);
                },
                empty_callable<nullptr_t>())(dbObjects);
        }

        template<class DBOs, satisfies<is_db_objects, DBOs> = true>
        std::string find_table_name(const DBOs& dbObjects, const std::type_index& ti) {
            std::string res;
            iterate_tuple<true>(dbObjects, tables_index_sequence<DBOs>{}, [&ti, &res](const auto& table) {
                using table_type = std::decay_t<decltype(table)>;
                if(ti == typeid(object_type_t<table_type>)) {
                    res = table.name;
                }
            });
            return res;
        }

        template<class Lookup, class DBOs, satisfies<is_db_objects, DBOs> = true>
        std::string lookup_table_name(const DBOs& dbObjects) {
            return static_if<is_mapped_v<DBOs, Lookup>>(
                [](const auto& dbObjects) {
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
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        constexpr decltype(auto) materialize_column_pointer(const DBOs&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class DBOs, satisfies<is_db_objects, DBOs> = true>
        const std::string* find_column_name(const DBOs& dbObjects, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(dbObjects, cp);
            return pick_table<O>(dbObjects).find_column_name(field);
        }
    }
}
