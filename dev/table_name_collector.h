#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <set>  //  std::set
#include <string>  //  std::string
#include <utility>  //  std::pair, std::move
#endif

#include "mapped_type_proxy.h"
#include "vocabulary/node_traits.h"
#include "rowid.h"
#include "alias.h"
#include "core_functions.h"
#include "storage_lookup.h"  // lookup_table_name

namespace sqlite_orm::internal {
    struct table_name_collector_base {
        using table_name_set = std::set<std::pair<std::string, std::string>>;

        table_name_set table_names;
    };

    template<class DBOs>
    struct table_name_collector : table_name_collector_base {
        using db_objects_type = DBOs;

        const db_objects_type& db_objects;

        table_name_collector(const db_objects_type& dbObjects) : db_objects{dbObjects} {}

        template<class ColRef>
        void operator()(const ColRef&) {
            if constexpr (std::is_member_pointer<ColRef>::value) {
                using table_type = table_type_of_t<ColRef>;
                auto tableName = lookup_table_name<mapped_type_proxy_t<table_type>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), "");
            }
            // ...
            else if constexpr (is_column_pointer_v<ColRef>) {
                using table_type = table_type_of_t<ColRef>;
                auto tableName = lookup_table_name<mapped_type_proxy_t<table_type>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), alias_extractor<table_type>::as_alias());
            }
            // ...
            else if constexpr (polyfill::is_specialization_of_v<ColRef, alias_column_t>) {
                // note: instead of accessing the column, we are interested in the type the column is aliased into
                using A = alias_type_t<ColRef>;
                auto tableName = lookup_table_name<mapped_type_proxy_t<A>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), alias_extractor<A>::as_alias());
            }
            // ...
            else if constexpr (polyfill::is_specialization_of_v<ColRef, count_asterisk_t>) {
                using table_type = type_t<ColRef>;
                auto tableName = lookup_table_name<table_type>(this->db_objects);
                if (!tableName.empty()) {
                    this->table_names.emplace(std::move(tableName), "");
                }
            }
            // ...
            else if constexpr (is_asterisk_v<ColRef>) {
                using recordset_type = type_t<ColRef>;
                auto tableName = lookup_table_name<mapped_type_proxy_t<recordset_type>>(this->db_objects);
                this->table_names.emplace(std::move(tableName), alias_extractor<recordset_type>::as_alias());
            }
            // ...
            else if constexpr (is_object_node_v<ColRef> || polyfill::is_specialization_of_v<ColRef, table_rowid_t> ||
                               polyfill::is_specialization_of_v<ColRef, table_oid_t> ||
                               polyfill::is_specialization_of_v<ColRef, table__rowid_t>) {
                using table_type = type_t<ColRef>;
                this->table_names.emplace(lookup_table_name<table_type>(this->db_objects), "");
            }
            // ...
            else {
                // Do nothing for other types of expressions
            }
        }

        /*  
         *  Invoked by the AST iterator for the node itself
         */
        template<class ColRef>
        void operator()(std::true_type, const ColRef&) {
            // ...
            if constexpr (polyfill::is_specialization_of_v<ColRef, highlight_t>) {
                using table_type = typename ColRef::table_type;
                this->table_names.emplace(lookup_table_name<table_type>(this->db_objects), "");
            }
            // ...
            else {
                // Do nothing for other types of expressions
            }
        }
    };
}
