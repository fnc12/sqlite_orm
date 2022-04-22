#pragma once

#include <string>  //  std::string
#include <sqlite3.h>
#include <sstream>  //  std::stringstream
#include <type_traits>  //  std::forward, std::is_same
#include <vector>  //  std::vector
#include <typeindex>  //  std::type_index

#include "static_magic.h"
#include "type_traits.h"
#include "constraints.h"
#include "table_info.h"
#include "sync_schema_result.h"
#include "storage_lookup.h"

namespace sqlite_orm {

    namespace internal {

        struct storage_impl_base {

            bool table_exists(const std::string& tableName, sqlite3* db) const;

            void rename_table(sqlite3* db, const std::string& oldName, const std::string& newName) const;

            static bool calculate_remove_add_columns(std::vector<table_xinfo*>& columnsToAdd,
                                                     std::vector<table_xinfo>& storageTableInfo,
                                                     std::vector<table_xinfo>& dbTableInfo);

            static void
            add_generated_cols(std::vector<table_xinfo*>& columnsToAdd,
                               std::vector<table_xinfo>& storageTableInfo);  // add generated columns to colummnsToAdd
        };

        /**
         *  This is a generic implementation. Used as a tail in storage_impl inheritance chain
         */
        template<class... Ts>
        struct storage_impl;

        template<class H, class... Ts>
        struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
            using super = storage_impl<Ts...>;
            using self = storage_impl<H, Ts...>;
            using table_type = H;

            storage_impl(H h, Ts... ts) : super(std::forward<Ts>(ts)...), table(std::move(h)) {}

            table_type table;

            template<class L>
            void for_each(const L& l) const {
                this->super::for_each(l);
                l(*this);
            }

#if SQLITE_VERSION_NUMBER >= 3006019

            /**
             *  Returns foreign keys count in table definition
             */
            int foreign_keys_count() const {
                auto res = 0;
                iterate_tuple(this->table.elements, [&res](auto& c) {
                    if(is_foreign_key<typename std::decay<decltype(c)>::type>::value) {
                        ++res;
                    }
                });
                return res;
            }

#endif

            std::string find_table_name(const std::type_index& ti) const {
                std::type_index thisTypeIndex{typeid(object_type_t<H>)};

                if(thisTypeIndex == ti) {
                    return this->table.name;
                } else {
                    return this->super::find_table_name(ti);
                }
            }

            /**
             *  Copies current table to another table with a given **name**.
             *  Performs CREATE TABLE %name% AS SELECT %this->table.columns_names()% FROM &this->table.name%;
             */
            void
            copy_table(sqlite3* db, const std::string& name, const std::vector<table_xinfo*>& columnsToIgnore) const;
        };

        template<>
        struct storage_impl<> : storage_impl_base {

            std::string find_table_name(const std::type_index&) const {
                return {};
            }

            template<class L>
            void for_each(const L&) const {}

            int foreign_keys_count() const {
                return 0;
            }
        };
    }
}

#include "static_magic.h"
#include "select_constraints.h"

// interface functions
namespace sqlite_orm {
    namespace internal {

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        auto lookup_table(const S& strg) {
            const auto& tImpl = find_impl<Lookup>(strg);
            return static_if<std::is_same<decltype(tImpl), const storage_impl<>&>{}>(
                [](const storage_impl<>&) {
                    return nullptr;
                },
                [](const auto& tImpl) {
                    return &tImpl.table;
                })(tImpl);
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        std::string lookup_table_name(const S& strg) {
            const auto& tImpl = find_impl<Lookup>(strg);
            return static_if<std::is_same<decltype(tImpl), const storage_impl<>&>{}>(
                [](const storage_impl<>&) {
                    return std::string{};
                },
                [](const auto& tImpl) {
                    return tImpl.table.name;
                })(tImpl);
        }

        template<class Lookup, class S, satisfies<is_storage_impl, S> = true>
        const std::string& get_table_name(const S& strg) {
            return pick_impl<Lookup>(strg).table.name;
        }

        /**
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, F O::*field) {
            return pick_impl<O>(strg).table.find_column_name(field);
        }

        /**
         *  Materialize column pointer:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&, const column_pointer<O, F>& cp) {
            return cp.field;
        }

        /**
         *  Find column name by:
         *  1. by explicit object type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, const column_pointer<O, F>& cp) {
            auto field = materialize_column_pointer(strg, cp);
            return pick_impl<O>(strg).table.find_column_name(field);
        }
    }
}
