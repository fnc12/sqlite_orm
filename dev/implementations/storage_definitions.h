/** @file Mainly existing to disentangle implementation details from circular and cross dependencies
 *  this file is also used to separate implementation details from the main header file,
 *  e.g. usage of the dbstat table.
 */
#pragma once
#include <type_traits>  //  std::is_same
#include <sstream>
#include <functional>  //  std::reference_wrapper, std::cref
#include <algorithm>  //  std::find_if

#include "../dbstat.h"
#include "../util.h"
#include "../serializing_util.h"
#include "../storage.h"

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
            bool attempt_to_preserve = true;

            auto schema_stat = this->schema_status(tImpl, db, preserve, &attempt_to_preserve);
            if(schema_stat != sync_schema_result::already_in_sync) {
                if(schema_stat == sync_schema_result::new_table_created) {
                    this->create_table(db, tImpl.table.name, tImpl);
                    res = sync_schema_result::new_table_created;
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

                        this->calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

                        if(schema_stat == sync_schema_result::old_columns_removed) {
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
                            for(auto& tableInfo: dbTableInfo) {
                                this->drop_column(db, tImpl.table.name, tableInfo.name);
                            }
                            res = sync_schema_result::old_columns_removed;
#else
                            //  extra table columns than storage columns
                            this->backup_table(db, tImpl, {});
                            res = sync_schema_result::old_columns_removed;
#endif
                        }

                        if(schema_stat == sync_schema_result::new_columns_added) {
                            for(const table_xinfo* colInfo: columnsToAdd) {
                                tImpl.table.for_each_column([this, colInfo, &tImpl, db](auto& column) {
                                    if(column.name != colInfo->name) {
                                        return;
                                    }
                                    this->add_column(tImpl.table.name, column, db);
                                });
                            }
                            res = sync_schema_result::new_columns_added;
                        }

                        if(schema_stat == sync_schema_result::new_columns_added_and_old_columns_removed) {

                            auto storageTableInfo = tImpl.table.get_table_info();
                            this->add_generated_cols(columnsToAdd, storageTableInfo);

                            // remove extra columns and generated columns
                            this->backup_table(db, tImpl, columnsToAdd);
                            res = sync_schema_result::new_columns_added_and_old_columns_removed;
                        }
                    } else if(schema_stat == sync_schema_result::dropped_and_recreated) {
                        //  now get current table info from db using `PRAGMA table_xinfo` query..
                        auto dbTableInfo =
                            this->pragma.table_xinfo(tImpl.table.name);  // should include generated columns
                        auto storageTableInfo = tImpl.table.get_table_info();

                        //  this vector will contain pointers to columns that gotta be added..
                        std::vector<const table_xinfo*> columnsToAdd;

                        this->calculate_remove_add_columns(columnsToAdd, storageTableInfo, dbTableInfo);

                        this->add_generated_cols(columnsToAdd, storageTableInfo);

                        if(preserve && attempt_to_preserve) {
                            this->backup_table(db, tImpl, columnsToAdd);
                        } else {
                            this->drop_create_with_loss(db, tImpl);
                        }
                        res = schema_stat;
                    }
                }
            }
            return res;
        }

        template<class... Ts>
        template<class I>
        void storage_t<Ts...>::copy_table(
            sqlite3* db,
            const std::string& sourceTableName,
            const std::string& destinationTableName,
            const I& tImpl,
            const std::vector<const table_xinfo*>& columnsToIgnore) const {  // must ignore generated columns
            std::vector<std::reference_wrapper<const std::string>> columnNames;
            columnNames.reserve(tImpl.table.count_columns_amount());
            tImpl.table.for_each_column([&columnNames, &columnsToIgnore](const auto& c) {
                auto& columnName = c.name;
                auto columnToIgnoreIt = std::find_if(columnsToIgnore.begin(),
                                                     columnsToIgnore.end(),
                                                     [&columnName](const table_xinfo* tableInfo) {
                                                         return columnName == tableInfo->name;
                                                     });
                if(columnToIgnoreIt == columnsToIgnore.end()) {
                    columnNames.push_back(cref(columnName));
                }
            });

            std::stringstream ss;
            ss << "INSERT INTO " << streaming_identifier(destinationTableName) << " ("
               << streaming_identifiers(columnNames) << ") "
               << "SELECT " << streaming_identifiers(columnNames) << " FROM " << streaming_identifier(sourceTableName)
               << std::flush;
            perform_void_exec(db, ss.str());
        }
    }
}
