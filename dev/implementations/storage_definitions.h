/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  this file is also used to separate implementation details from the main header file,
 *  e.g. usage of the dbstat table.
 */
#pragma once
#include <type_traits>  //  std::is_same

#include "../storage.h"
#include "../dbstat.h"
#include "../util.h"

namespace sqlite_orm {
    namespace internal {

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

            auto schema_stat = this->schema_status(tImpl, db, preserve);
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

                        //  now get current table info from db using `PRAGMA table_xinfo` query..
                        auto dbTableInfo =
                            this->pragma.table_xinfo(tImpl.table.name);  // should include generated columns

                        //  this vector will contain pointers to columns that gotta be added..
                        std::vector<const table_xinfo*> columnsToAdd;

                        calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

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

                            auto storageTableInfo = tImpl.table.get_table_info();
                            add_generated_cols(columnsToAdd, storageTableInfo);

                            // remove extra columns and generated columns
                            this->backup_table(db, tImpl, columnsToAdd);
                            res = decltype(res)::new_columns_added_and_old_columns_removed;
                        }
                    } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                        //  now get current table info from db using `PRAGMA table_xinfo` query..
                        auto dbTableInfo =
                            this->pragma.table_xinfo(tImpl.table.name);  // should include generated columns
                        auto storageTableInfo = tImpl.table.get_table_info();

                        //  this vector will contain pointers to columns that gotta be added..
                        std::vector<const table_xinfo*> columnsToAdd;

                        calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

                        add_generated_cols(columnsToAdd, storageTableInfo);

                        this->backup_table(db, tImpl, columnsToAdd);
                        res = decltype(res)::dropped_and_recreated;
                    } else if(schema_stat == sync_schema_result::table_data_loss) {
                        this->drop_create_with_loss(tImpl, db);
                        res = decltype(res)::table_data_loss;
                    }
                }
            }
            return res;
        }

        template<class... Ts>
        template<class I>
        void storage_t<Ts...>::copy_table(
            sqlite3* db,
            const std::string& tableName,
            const I& tImpl,
            const std::vector<const table_xinfo*>& columnsToIgnore) const {  // must ignore generated columns
            std::stringstream ss;
            std::vector<std::string> columnNames;
            tImpl.table.for_each_column([&columnNames, &columnsToIgnore](auto& c) {
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
            ss << "INSERT INTO " << quote_identifier(tableName) << " (";
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
            ss << " FROM " << quote_identifier(tImpl.table.name);
            perform_void_exec(db, ss.str());
        }

        // jdh
        template<class... Ts>
        template<class I>
        void storage_t<Ts...>::copy_table_from(
            sqlite3* db,
            const std::string& tableName,
            const I& tImpl,
            const std::vector<const table_xinfo*>& columnsToIgnore) const {  // must ignore generated columns
            std::stringstream ss;
            std::vector<std::string> columnNames;
            tImpl.table.for_each_column([&columnNames, &columnsToIgnore](auto& c) {
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
            ss << "INSERT INTO " << quote_identifier(tImpl.table.name) << " (";
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
            ss << " FROM " << quote_identifier(tableName);
            perform_void_exec(db, ss.str());
        }

        template<class... Ts>
        inline bool storage_t<Ts...>::calculate_remove_add_columns(std::vector<const table_xinfo*>& columnsToAdd,
                                                                   std::vector<table_xinfo>& storageTableInfo,
                                                                   std::vector<table_xinfo>& dbTableInfo) {
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
                        dbColumnInfo.pk == storageColumnInfo.pk &&
                        (dbColumnInfo.hidden == 0) == (storageColumnInfo.hidden == 0);
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
    }
}
