/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  (e.g. column_t -> default_value_extractor -> serializator_context -> storage_impl -> table_t -> column_t)
 *  this file is also used to provide definitions of interface methods 'hitting the database'.
 */
#pragma once
#include <sqlite3.h>
#include <type_traits>  //  std::is_same
#include <utility>  //  std::move
#include <algorithm>  //  std::find_if
#include <sstream>  //  std::stringstream
#include <cstdlib>  //  std::atoi

#include "error_code.h"
#include "default_value_extractor.h"
#include "type_printer.h"
#include "column.h"
#include "table.h"
#include "storage_impl.h"
#include "storage.h"
#include "dbstat.h"
#include "util.h"

namespace sqlite_orm {
    namespace internal {

        template<class O, class T, class G, class S, class... Op>
        std::unique_ptr<std::string> column_t<O, T, G, S, Op...>::default_value() const {
            std::unique_ptr<std::string> res;
            iterate_tuple(this->constraints, [&res](auto& v) {
                if(auto dft = default_value_extractor{}(v)) {
                    res = move(dft);
                }
            });
            return res;
        }

        template<class T, bool WithoutRowId, class... Cs>
        std::vector<table_info> table_t<T, WithoutRowId, Cs...>::get_table_info() const {
            std::vector<table_info> res;
            res.reserve(size_t(this->elements_count));
            this->for_each_column([&res](auto& col) {
                using field_type = field_type_t<std::decay_t<decltype(col)>>;
                std::string dft;
                if(auto d = col.default_value()) {
                    dft = move(*d);
                }
                res.emplace_back(-1,
                                 col.name,
                                 type_printer<field_type>().print(),
                                 col.not_null(),
                                 dft,
                                 col.template has<primary_key_t<>>());
            });
            auto compositeKeyColumnNames = this->composite_key_columns_names();
            for(size_t i = 0; i < compositeKeyColumnNames.size(); ++i) {
                auto& columnName = compositeKeyColumnNames[i];
                auto it = std::find_if(res.begin(), res.end(), [&columnName](const table_info& ti) {
                    return ti.name == columnName;
                });
                if(it != res.end()) {
                    it->pk = static_cast<int>(i + 1);
                }
            }
            return res;
        }

        inline bool storage_impl_base::table_exists(const std::string& tableName, sqlite3* db) const {
            bool result = false;
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
                throw_translated_sqlite_error(db);
            }
            return result;
        }

        inline void
        storage_impl_base::rename_table(sqlite3* db, const std::string& oldName, const std::string& newName) const {
            std::stringstream ss;
            ss << "ALTER TABLE " << oldName << " RENAME TO " << newName;
            perform_void_exec(db, ss.str());
        }

        inline bool storage_impl_base::calculate_remove_add_columns(std::vector<table_info*>& columnsToAdd,
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
                        (!dbColumnInfo.dflt_value.empty()) == (!storageColumnInfo.dflt_value.empty()) &&
                        dbColumnInfo.pk == storageColumnInfo.pk;
                    if(!columnsAreEqual) {
                        notEqual = true;
                        break;
                    }
                    dbTableInfo.erase(dbColumnInfoIt);
                    storageTableInfo.erase(storageTableInfo.begin() + static_cast<ptrdiff_t>(storageColumnInfoIndex));
                    --storageColumnInfoIndex;
                } else {
                    columnsToAdd.push_back(&storageColumnInfo);
                }
            }
            return notEqual;
        }

        inline std::vector<table_info> storage_impl_base::get_table_info(const std::string& tableName,
                                                                         sqlite3* db) const {
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
                        res.emplace_back(cid, name, type, notnull, dflt_value, pk);
                    }
                    return 0;
                },
                &result,
                nullptr);
            if(rc != SQLITE_OK) {
                throw_translated_sqlite_error(db);
            }
            return result;
        }

        template<class H, class... Ts>
        void storage_impl<H, Ts...>::copy_table(sqlite3* db,
                                                const std::string& name,
                                                const std::vector<table_info*>& columnsToIgnore) const {
            std::stringstream ss;
            std::vector<std::string> columnNames;
            this->table.for_each_column([&columnNames, &columnsToIgnore](auto& c) {
                auto& columnName = c.name;
                auto columnToIgnoreIt =
                    std::find_if(columnsToIgnore.begin(), columnsToIgnore.end(), [&columnName](auto tableInfoPointer) {
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

        template<class H, class... Ts>
        sync_schema_result storage_impl<H, Ts...>::schema_status(sqlite3* db, bool preserve) const {

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
                    if(!dbTableInfo.empty()) {
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
                    if(!columnsToAdd.empty()) {
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

        template<class... Ts>
        template<class T, bool WithoutRowId, class... Args, class... Tss>
        sync_schema_result
        storage_t<Ts...>::sync_table(const storage_impl<table_t<T, WithoutRowId, Args...>, Tss...>& tImpl,
                                     sqlite3* db,
                                     bool preserve) {
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
            if(std::is_same<T, dbstat>::value) {
                return sync_schema_result::already_in_sync;
            }
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
            auto res = sync_schema_result::already_in_sync;

            auto schema_stat = tImpl.schema_status(db, preserve);
            if(schema_stat != decltype(schema_stat)::already_in_sync) {
                if(schema_stat == decltype(schema_stat)::new_table_created) {
                    this->create_table(db, tImpl.table.name, tImpl);
                    res = decltype(res)::new_table_created;
                } else {
                    if(schema_stat == sync_schema_result::old_columns_removed ||
                       schema_stat == sync_schema_result::new_columns_added ||
                       schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {

                        //  get table info provided in `make_table` call..
                        auto storageTableInfo = tImpl.table.get_table_info();

                        //  now get current table info from db using `PRAGMA table_info` query..
                        auto dbTableInfo = tImpl.get_table_info(tImpl.table.name, db);

                        //  this vector will contain pointers to columns that gotta be added..
                        std::vector<table_info*> columnsToAdd;

                        tImpl.calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

                        if(schema_stat == sync_schema_result::old_columns_removed) {
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
                            for(auto& tableInfo: dbTableInfo) {
                                this->drop_column(db, tImpl.table.name, tableInfo.name);
                            }
                            res = decltype(res)::old_columns_removed;
#else
                            //  extra table columns than storage columns
                            this->backup_table(db, tImpl, {});
                            res = decltype(res)::old_columns_removed;
#endif
                        }

                        if(schema_stat == sync_schema_result::new_columns_added) {
                            for(auto columnPointer: columnsToAdd) {
                                tImpl.table.for_each_column([this, columnPointer, &tImpl, db](auto& column) {
                                    if(column.name != columnPointer->name) {
                                        return;
                                    }
                                    this->add_column(tImpl.table.name, column, db);
                                });
                            }
                            res = decltype(res)::new_columns_added;
                        }

                        if(schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {

                            // remove extra columns
                            this->backup_table(db, tImpl, columnsToAdd);
                            res = decltype(res)::new_columns_added_and_old_columns_removed;
                        }
                    } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                        this->drop_table_internal(tImpl.table.name, db);
                        this->create_table(db, tImpl.table.name, tImpl);
                        res = decltype(res)::dropped_and_recreated;
                    }
                }
            }
            return res;
        }

    }
}
