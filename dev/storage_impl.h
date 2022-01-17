#pragma once

#include <string>  //  std::string
#include <sqlite3.h>
#include <cstddef>  //  std::nullptr_t
#include <system_error>  //  std::system_error, std::error_code
#include <sstream>  //  std::stringstream
#include <stdlib.h>  //  std::atoi
#include <type_traits>  //  std::forward, std::enable_if, std::is_same, std::remove_reference, std::false_type, std::true_type
#include <utility>  //  std::pair, std::make_pair
#include <vector>  //  std::vector
#include <algorithm>  //  std::find_if
#include <typeindex>  //  std::type_index

#include "type_traits.h"
#include "error_code.h"
#include "statement_finalizer.h"
#include "row_extractor.h"
#include "util.h"
#include "constraints.h"
#include "select_constraints.h"
#include "field_printer.h"
#include "table_info.h"
#include "sync_schema_result.h"
#include "field_value_holder.h"

namespace sqlite_orm {

    namespace internal {

        struct storage_impl_base {

            bool table_exists(const std::string& tableName, sqlite3* db) const {
                auto result = false;
                std::stringstream ss;
                ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '"
                   << "table"
                   << "' AND name = '" << tableName << "'";
                auto query = ss.str();
                auto rc = sqlite3_exec(
                    db,
                    query.c_str(),
                    [](void* data, int argc, char** argv, char** /*azColName*/) -> int {
                        auto& res = *(bool*)data;
                        if(argc) {
                            res = !!std::atoi(argv[0]);
                        }
                        return 0;
                    },
                    &result,
                    nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return result;
            }

            void rename_table(sqlite3* db, const std::string& oldName, const std::string& newName) const {
                std::stringstream ss;
                ss << "ALTER TABLE " << oldName << " RENAME TO " << newName;
                perform_void_exec(db, ss.str());
            }

            static bool calculate_remove_add_columns(std::vector<table_info*>& columnsToAdd,
                                                     std::vector<table_info>& storageTableInfo,
                                                     std::vector<table_info>& dbTableInfo) {
                bool notEqual = false;

                //  iterate through storage columns
                for(size_t storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size();
                    ++storageColumnInfoIndex) {

                    //  get storage's column info
                    auto& storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                    auto& columnName = storageColumnInfo.name;

                    //  search for a column in db eith the same name
                    auto dbColumnInfoIt = std::find_if(dbTableInfo.begin(), dbTableInfo.end(), [&columnName](auto& ti) {
                        return ti.name == columnName;
                    });
                    if(dbColumnInfoIt != dbTableInfo.end()) {
                        auto& dbColumnInfo = *dbColumnInfoIt;
                        auto columnsAreEqual =
                            dbColumnInfo.name == storageColumnInfo.name &&
                            dbColumnInfo.notnull == storageColumnInfo.notnull &&
                            (dbColumnInfo.dflt_value.length() > 0) == (storageColumnInfo.dflt_value.length() > 0) &&
                            dbColumnInfo.pk == storageColumnInfo.pk;
                        if(!columnsAreEqual) {
                            notEqual = true;
                            break;
                        }
                        dbTableInfo.erase(dbColumnInfoIt);
                        storageTableInfo.erase(storageTableInfo.begin() +
                                               static_cast<ptrdiff_t>(storageColumnInfoIndex));
                        --storageColumnInfoIndex;
                    } else {
                        columnsToAdd.push_back(&storageColumnInfo);
                    }
                }
                return notEqual;
            }

            std::vector<table_info> get_table_info(const std::string& tableName, sqlite3* db) const {
                std::vector<table_info> result;
                auto query = "PRAGMA table_info('" + tableName + "')";
                auto rc = sqlite3_exec(
                    db,
                    query.c_str(),
                    [](void* data, int argc, char** argv, char**) -> int {
                        auto& res = *(std::vector<table_info>*)data;
                        if(argc) {
                            auto index = 0;
                            auto cid = std::atoi(argv[index++]);
                            std::string name = argv[index++];
                            std::string type = argv[index++];
                            bool notnull = !!std::atoi(argv[index++]);
                            std::string dflt_value = argv[index] ? argv[index] : "";
                            index++;
                            auto pk = std::atoi(argv[index++]);
                            res.push_back(table_info(cid, name, type, notnull, dflt_value, pk));
                        }
                        return 0;
                    },
                    &result,
                    nullptr);
                if(rc != SQLITE_OK) {
                    throw std::system_error(std::error_code(sqlite3_errcode(db), get_sqlite_error_category()),
                                            sqlite3_errmsg(db));
                }
                return result;
            }
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
            void for_each(const L& l) {
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

            template<class Lookup>
            storage_pick_impl_t<self, Lookup>& get_impl() {
                return *this;
            }
            template<class Lookup>
            storage_pick_impl_t<const self, Lookup>& get_impl() const {
                return *this;
            }

            std::string find_table_name(std::type_index ti) const {
                std::type_index thisTypeIndex{typeid(std::conditional_t<std::is_void<cte_label_type_t<H>>::value,
                                                                        object_type_t<H>,
                                                                        cte_label_type_t<H>>)};
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
            copy_table(sqlite3* db, const std::string& name, const std::vector<table_info*>& columnsToIgnore) const {
                std::stringstream ss;
                std::vector<std::string> columnNames;
                this->table.for_each_column([&columnNames, &columnsToIgnore](auto& c) {
                    auto& columnName = c.name;
                    auto columnToIgnoreIt = std::find_if(columnsToIgnore.begin(),
                                                         columnsToIgnore.end(),
                                                         [&columnName](auto tableInfoPointer) {
                                                             return columnName == tableInfoPointer->name;
                                                         });
                    if(columnToIgnoreIt == columnsToIgnore.end()) {
                        columnNames.emplace_back(columnName);
                    }
                });
                auto columnNamesCount = columnNames.size();
                ss << "INSERT INTO " << name << " (";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << columnNames[i];
                    if(i < columnNamesCount - 1) {
                        ss << ",";
                    }
                    ss << " ";
                }
                ss << ") ";
                ss << "SELECT ";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << columnNames[i];
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }
                }
                ss << " FROM '" << this->table.name << "'";
                perform_void_exec(db, ss.str());
            }

            sync_schema_result schema_status(sqlite3* db, bool preserve) const {

                auto res = sync_schema_result::already_in_sync;

                //  first let's see if table with such name exists..
                auto gottaCreateTable = !this->table_exists(this->table.name, db);
                if(!gottaCreateTable) {

                    //  get table info provided in `make_table` call..
                    auto storageTableInfo = this->table.get_table_info();

                    //  now get current table info from db using `PRAGMA table_info` query..
                    auto dbTableInfo = this->get_table_info(this->table.name, db);

                    //  this vector will contain pointers to columns that gotta be added..
                    std::vector<table_info*> columnsToAdd;

                    if(this->calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo)) {
                        gottaCreateTable = true;
                    }

                    if(!gottaCreateTable) {  //  if all storage columns are equal to actual db columns but there are
                        //  excess columns at the db..
                        if(dbTableInfo.size() > 0) {
                            // extra table columns than storage columns
                            if(!preserve) {
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
                                res = decltype(res)::old_columns_removed;
#else
                                gottaCreateTable = true;
#endif
                            } else {
                                res = decltype(res)::old_columns_removed;
                            }
                        }
                    }
                    if(gottaCreateTable) {
                        res = decltype(res)::dropped_and_recreated;
                    } else {
                        if(columnsToAdd.size()) {
                            // extra storage columns than table columns
                            for(auto columnPointer: columnsToAdd) {
                                auto generatedStorageTypePointer =
                                    this->find_column_generated_storage_type(columnPointer->name);
                                if(generatedStorageTypePointer) {
                                    if(*generatedStorageTypePointer == basic_generated_always::storage_type::stored) {
                                        gottaCreateTable = true;
                                        break;
                                    }
                                    //  fallback cause VIRTUAL can be added
                                } else {
                                    if(columnPointer->notnull && columnPointer->dflt_value.empty()) {
                                        gottaCreateTable = true;
                                        break;
                                    }
                                }
                            }
                            if(!gottaCreateTable) {
                                if(res == decltype(res)::old_columns_removed) {
                                    res = decltype(res)::new_columns_added_and_old_columns_removed;
                                } else {
                                    res = decltype(res)::new_columns_added;
                                }
                            } else {
                                res = decltype(res)::dropped_and_recreated;
                            }
                        } else {
                            if(res != decltype(res)::old_columns_removed) {
                                res = decltype(res)::already_in_sync;
                            }
                        }
                    }
                } else {
                    res = decltype(res)::new_table_created;
                }
                return res;
            }

          private:
            const basic_generated_always::storage_type*
            find_column_generated_storage_type(const std::string& name) const {
                const basic_generated_always::storage_type* result = nullptr;
#if SQLITE_VERSION_NUMBER >= 3031000
                this->table.for_each_column([&result, &name](auto& column) {
                    if(column.name != name) {
                        return;
                    }
                    iterate_tuple(column.constraints, [&result](auto& constraint) {
                        if(result) {
                            return;
                        }
                        using constraint_type = typename std::decay<decltype(constraint)>::type;
                        static_if<is_generated_always<constraint_type>{}>([&result](auto& generatedAlwaysConstraint) {
                            result = &generatedAlwaysConstraint.storage;
                        })(constraint);
                    });
                });
#endif
                return result;
            }
        };

        template<>
        struct storage_impl<> : storage_impl_base {

            // Provided just to be on the safe side
            template<class Lookup>
            const storage_impl<>& get_impl() const {
                static_assert(polyfill::always_false_v<Lookup>, "No such storage implementation");
            }

            std::string find_table_name(std::type_index) const {
                return {};
            }

            template<class L>
            void for_each(const L&) {}

            int foreign_keys_count() const {
                return 0;
            }
        };
    }
}

#include "storage_lookup.h"

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
         *  Find column name by its type and member pointer.
         */
        template<class O, class F, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, const column_pointer<O, F>& cp) {
            return pick_impl<O>(strg).table.find_column_name(cp.field);
        }

        template<class Label, size_t I, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&,
                                                            const column_pointer<Label, polyfill::index_constant<I>>&) {
            using timpl_type = storage_pick_impl_t<S, Label>;
            return cte_getter_v<storage_object_type_t<timpl_type>, I>;
        }

        template<class Label, size_t I, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg,
                                            const column_pointer<Label, polyfill::index_constant<I>>& cp) {
            auto field = materialize_column_pointer(strg, cp);
            return pick_impl<Label>(strg).table.find_column_name(field);
        }

        template<class Label, class O, class F, F O::*m, class S, satisfies<is_storage_impl, S> = true>
        constexpr decltype(auto) materialize_column_pointer(const S&, const column_pointer<Label, ice_t<m>>&) {
            using timpl_type = storage_pick_impl_t<S, Label>;
            using cte_mapper_type = storage_cte_mapper_type_t<timpl_type>;
            constexpr auto I = tuple_index_of_v<ice_t<m>, typename cte_mapper_type::expressions_type>;
            return cte_getter_v<cte_object_type_t<cte_mapper_type>, I>;
        }

        template<class Label, class O, class F, F O::*m, class S, satisfies<is_storage_impl, S> = true>
        const std::string* find_column_name(const S& strg, const column_pointer<Label, ice_t<m>>& cp) {
            auto field = materialize_column_pointer(strg, cp);
            return pick_impl<Label>(strg).table.find_column_name(field);
        }
    }
}
