#pragma once

#include <set>  //  std::set
#include <string>  //  std::string
#include <utility>  //  std::pair, std::move

#include "functional/cxx_type_traits_polyfill.h"
#include "type_traits.h"
#include "select_constraints.h"
#include "alias.h"
#include "core_functions.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        struct table_name_collector_base {
            using table_name_set = std::set<std::pair<std::string, std::string>>;

            mutable table_name_set table_names;
        };

        template<class DBOs>
        struct table_name_collector : table_name_collector_base {
            using db_objects_type = DBOs;

            const db_objects_type& db_objects;

            table_name_collector() = default;

            table_name_collector(const db_objects_type& dbObjects) : db_objects{dbObjects} {}

            template<class T>
            void operator()(const T&) const {}

            template<class F, class O>
            void operator()(F O::*, std::string alias = {}) const {
                this->table_names.emplace(lookup_table_name<O>(this->db_objects), std::move(alias));
            }

            template<class T, class F>
            void operator()(const column_pointer<T, F>&) const {
                this->table_names.emplace(lookup_table_name<T>(this->db_objects), "");
            }

            template<class A, class C>
            void operator()(const alias_column_t<A, C>&) const {
                // note: instead of accessing the column, we are interested in the type the column is aliased into
                auto tableName = lookup_table_name<mapped_type_proxy_t<A>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), alias_extractor<A>::get());
            }

            template<class T>
            void operator()(const count_asterisk_t<T>&) const {
                auto tableName = lookup_table_name<T>(this->db_objects);
                if(!tableName.empty()) {
                    this->table_names.emplace(std::move(tableName), "");
                }
            }

            template<class T, satisfies_not<std::is_base_of, alias_tag, T> = true>
            void operator()(const asterisk_t<T>&) const {
                this->table_names.emplace(lookup_table_name<T>(db_objects), "");
            }

            template<class T, satisfies<std::is_base_of, alias_tag, T> = true>
            void operator()(const asterisk_t<T>&) const {
                // note: not all alias classes have a nested A::type
                static_assert(polyfill::is_detected_v<type_t, T>,
                              "alias<O> must have a nested alias<O>::type typename");
                auto tableName = lookup_table_name<type_t<T>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), alias_extractor<T>::get());
            }

            template<class T>
            void operator()(const object_t<T>&) const {
                this->table_names.emplace(lookup_table_name<T>(this->db_objects), "");
            }

            template<class T>
            void operator()(const table_rowid_t<T>&) const {
                this->table_names.emplace(lookup_table_name<T>(this->db_objects), "");
            }

            template<class T>
            void operator()(const table_oid_t<T>&) const {
                this->table_names.emplace(lookup_table_name<T>(this->db_objects), "");
            }

            template<class T>
            void operator()(const table__rowid_t<T>&) const {
                this->table_names.emplace(lookup_table_name<T>(this->db_objects), "");
            }
        };

        template<class DBOs, satisfies<is_db_objects, DBOs> = true>
        table_name_collector<DBOs> make_table_name_collector(const DBOs& dbObjects) {
            return {dbObjects};
        }

    }

}
