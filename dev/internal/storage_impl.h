//
//  storage_impl.h
//  CPPTest
//
//  Created by John Zakharov on 21.01.2018.
//  Copyright © 2018 John Zakharov. All rights reserved.
//

#ifndef storage_impl_h
#define storage_impl_h

#include <sqlite3.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>

#include "internal.h"

namespace sqlite_orm {
    
    namespace internal {
        
        /**
         *  This is a generic implementation. Used as a tail in storage_impl inheritance chain
         */
        template<class ...Ts>
        struct storage_impl {
            
            template<class L>
            void for_each(L) {}
            
            int foreign_keys_count() {
                return 0;
            }
            
            template<class O>
            std::string dump(const O &, sqlite3 *, std::nullptr_t) {
                throw std::runtime_error("type " + std::string(typeid(O).name()) + " is not mapped to storage in max");
            }
            
            bool table_exists(const std::string &tableName, sqlite3 *db) {
                auto res = false;
                std::stringstream ss;
                ss << "SELECT COUNT(*) FROM sqlite_master WHERE type = '" << "table" << "' AND name = '" << tableName << "'";
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **/*azColName*/) -> int {
                                           auto &res = *(bool*)data;
                                           if(argc){
                                               res = !!std::atoi(argv[0]);
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
            void begin_transaction(sqlite3 *db) {
                std::stringstream ss;
                ss << "BEGIN TRANSACTION";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            void commit(sqlite3 *db) {
                std::stringstream ss;
                ss << "COMMIT";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            void rollback(sqlite3 *db) {
                std::stringstream ss;
                ss << "ROLLBACK";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            void rename_table(sqlite3 *db, const std::string &oldName, const std::string &newName) {
                std::stringstream ss;
                ss << "ALTER TABLE " << oldName << " RENAME TO " << newName;
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        //  done..
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            std::string current_timestamp(sqlite3 *db) {
                std::string res;
                std::stringstream ss;
                ss << "SELECT CURRENT_TIMESTAMP";
                auto query = ss.str();
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv, char **) -> int {
                                           auto &res = *(std::string*)data;
                                           if(argc){
                                               if(argv[0]){
                                                   res = row_extractor<std::string>().extract(argv[0]);
                                               }
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
                return res;
            }
        };
        
        template<class H, class ...Ts>
        struct storage_impl<H, Ts...> : public storage_impl<Ts...> {
            typedef H table_type;
            
            storage_impl(H h, Ts ...ts) : Super(ts...), table(h) {}
            
            table_type table;
            
            template<class L>
            void for_each(L l) {
                Super::for_each(l);
                l(this);
            }
            
#if SQLITE_VERSION_NUMBER >= 3006019
            
            //  returns foreign keys count in table definition
            int foreign_keys_count(){
                auto res = 0;
                this->table.for_each_column_with_constraints([&res](auto c){
                    if(internal::is_foreign_key<decltype(c)>::value) {
                        ++res;
                    }
                });
                return res;
            }
            
#endif
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(F O::*m, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                return this->table.find_column_name(m);
            }
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(F O::*m, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return Super::column_name(m);
            }
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(const F& (O::*g)() const, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                return this->table.find_column_name(g);
            }
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(const F& (O::*g)() const, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return Super::column_name(g);
            }
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(void (O::*s)(F), typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                return this->table.find_column_name(s);
            }
            
            template<class O, class F, class HH = typename H::object_type>
            std::string column_name(void (O::*s)(F), typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return Super::column_name(s);
            }
            
            template<class O, class HH = typename H::object_type>
            auto& get_impl(typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                return *this;
            }
            
            template<class O, class HH = typename H::object_type>
            auto& get_impl(typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return Super::template get_impl<O>();
            }
            
            template<class O, class HH = typename H::object_type>
            std::string find_table_name(typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                return this->table.name;
            }
            
            template<class O, class HH = typename H::object_type>
            std::string find_table_name(typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return this->Super::template find_table_name<O>();
            }
            
            template<class O, class HH = typename H::object_type>
            std::string dump(const O &o, sqlite3 *db, typename std::enable_if<!std::is_same<O, HH>::value>::type * = nullptr) {
                return Super::dump(o, db, nullptr);
            }
            
            template<class O, class HH = typename H::object_type>
            std::string dump(const O &o, sqlite3 *, typename std::enable_if<std::is_same<O, HH>::value>::type * = nullptr) {
                std::stringstream ss;
                ss << "{ ";
                std::vector<std::pair<std::string, std::string>> pairs;
                this->table.for_each_column([&pairs, &o] (auto c) {
                    typedef typename decltype(c)::field_type field_type;
                    const field_type *value = nullptr;
                    if(c.member_pointer){
                        value = &(o.*c.member_pointer);
                    }else{
                        value = &((o).*(c.getter))();
                    }
                    pairs.push_back(std::make_pair(c.name, field_printer<field_type>()(*value)));
                });
                for(size_t i = 0; i < pairs.size(); ++i) {
                    auto &p = pairs[i];
                    ss << p.first << " : '" << p.second << "'";
                    if(i < pairs.size() - 1) {
                        ss << ", ";
                    }else{
                        ss << " }";
                    }
                }
                return ss.str();
            }
            
            std::vector<table_info> get_table_info(const std::string &tableName, sqlite3 *db) {
                std::vector<table_info> res;
                auto query = "PRAGMA table_info('" + tableName + "')";
                auto rc = sqlite3_exec(db,
                                       query.c_str(),
                                       [](void *data, int argc, char **argv,char **) -> int {
                                           auto &res = *(std::vector<table_info>*)data;
                                           if(argc){
                                               auto index = 0;
                                               auto cid = std::atoi(argv[index++]);
                                               std::string name = argv[index++];
                                               std::string type = argv[index++];
                                               bool notnull = !!std::atoi(argv[index++]);
                                               std::string dflt_value = argv[index] ? argv[index] : "";
                                               index++;
                                               auto pk = std::atoi(argv[index++]);
                                               res.push_back(table_info{cid, name, type, notnull, dflt_value, pk});
                                           }
                                           return 0;
                                       }, &res, nullptr);
                if(rc != SQLITE_OK) {
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
                return res;
            }
            
            void add_column(table_info &ti, sqlite3 *db) {
                std::stringstream ss;
                ss << "ALTER TABLE " << this->table.name << " ADD COLUMN " << ti.name << " ";
                ss << ti.type << " ";
                if(ti.pk){
                    ss << "PRIMARY KEY ";
                }
                if(ti.notnull){
                    ss << "NOT NULL ";
                }
                if(ti.dflt_value.length()) {
                    ss << "DEFAULT " << ti.dflt_value << " ";
                }
                auto query = ss.str();
                sqlite3_stmt *stmt;
                auto prepareResult = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
                if (prepareResult == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            /**
             *  Copies current table to another table with a given **name**.
             *  Performs CREATE TABLE %name% AS SELECT %this->table.columns_names()% FROM &this->table.name%;
             */
            void copy_table(sqlite3 *db, const std::string &name) {
                std::stringstream ss;
                std::vector<std::string> columnNames;
                this->table.for_each_column([&] (auto c) {
                    columnNames.emplace_back(c.name);
                });
                auto columnNamesCount = columnNames.size();
                ss << "INSERT INTO " << name << " (";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << columnNames[i];
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << ") ";
                ss << "SELECT ";
                for(size_t i = 0; i < columnNamesCount; ++i) {
                    ss << columnNames[i];
                    if(i < columnNamesCount - 1) {
                        ss << ", ";
                    }else{
                        ss << " ";
                    }
                }
                ss << " FROM '" << this->table.name << "' ";
                auto query = ss.str();
                sqlite3_stmt *stmt;
                if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                    statement_finalizer finalizer{stmt};
                    if (sqlite3_step(stmt) == SQLITE_DONE) {
                        return;
                    }else{
                        auto msg = sqlite3_errmsg(db);
                        throw std::runtime_error(msg);
                    }
                }else{
                    auto msg = sqlite3_errmsg(db);
                    throw std::runtime_error(msg);
                }
            }
            
            sync_schema_result schema_status(sqlite3 *db, bool preserve) {
                
                auto res = sync_schema_result::already_in_sync;
                
                //  first let's see if table with such name exists..
                auto gottaCreateTable = !this->table_exists(this->table.name, db);
                if(!gottaCreateTable){
                    
                    //  get table info provided in `make_table` call..
                    auto storageTableInfo = this->table.get_table_info();
                    
                    //  now get current table info from db using `PRAGMA table_info` query..
                    auto dbTableInfo = get_table_info(this->table.name, db);
                    
                    //  this vector will contain pointers to columns that gotta be added..
                    std::vector<table_info*> columnsToAdd;
                    
                    if(get_remove_add_columns(columnsToAdd,
                                              storageTableInfo,
                                              dbTableInfo)) {
                        gottaCreateTable = true;
                    }
                    
                    if(!gottaCreateTable){  //  if all storage columns are equal to actual db columns but there are excess columns at the db..
                        if(dbTableInfo.size() > 0){
                            //extra table columns than storage columns
                            if(!preserve){
                                gottaCreateTable = true;
                            }else{
                                res = decltype(res)::old_columns_removed;
                            }
                        }
                    }
                    if(gottaCreateTable){
                        res = decltype(res)::dropped_and_recreated;
                    }else{
                        if(columnsToAdd.size()){
                            //extra storage columns than table columns
                            for(auto columnPointer : columnsToAdd) {
                                if(columnPointer->notnull && columnPointer->dflt_value.empty()){
                                    gottaCreateTable = true;
                                    break;
                                }
                            }
                            if(!gottaCreateTable){
                                if(res == decltype(res)::old_columns_removed) {
                                    res = decltype(res)::new_columns_added_and_old_columns_removed;
                                }else{
                                    res = decltype(res)::new_columns_added;
                                }
                            }else{
                                res = decltype(res)::dropped_and_recreated;
                            }
                        }else{
                            if(res != decltype(res)::old_columns_removed){
                                res = decltype(res)::already_in_sync;
                            }
                        }
                    }
                }else{
                    res = decltype(res)::new_table_created;
                }
                return res;
            }
            
            static bool get_remove_add_columns(std::vector<table_info*>& columnsToAdd,
                                               std::vector<table_info>& storageTableInfo,
                                               std::vector<table_info>& dbTableInfo)
            {
                bool notEqual = false;
                
                //  iterate through storage columns
                for(size_t storageColumnInfoIndex = 0; storageColumnInfoIndex < storageTableInfo.size(); ++storageColumnInfoIndex) {
                    
                    //  get storage's column info
                    auto &storageColumnInfo = storageTableInfo[storageColumnInfoIndex];
                    auto &columnName = storageColumnInfo.name;
                    
                    //  search for a column in db eith the same name
                    auto dbColumnInfoIt = std::find_if(dbTableInfo.begin(),
                                                       dbTableInfo.end(),
                                                       [&](auto &ti){
                                                           return ti.name == columnName;
                                                       });
                    if(dbColumnInfoIt != dbTableInfo.end()){
                        auto &dbColumnInfo = *dbColumnInfoIt;
                        auto dbColumnInfoType = to_sqlite_type(dbColumnInfo.type);
                        auto storageColumnInfoType = to_sqlite_type(storageColumnInfo.type);
                        if(dbColumnInfoType && storageColumnInfoType) {
                            auto columnsAreEqual = dbColumnInfo.name == storageColumnInfo.name &&
                            *dbColumnInfoType == *storageColumnInfoType &&
                            dbColumnInfo.notnull == storageColumnInfo.notnull &&
                            bool(dbColumnInfo.dflt_value.length()) == bool(storageColumnInfo.dflt_value.length()) &&
                            dbColumnInfo.pk == storageColumnInfo.pk;
                            if(!columnsAreEqual){
                                notEqual = true;
                                break;
                            }
                            dbTableInfo.erase(dbColumnInfoIt);
                            storageTableInfo.erase(storageTableInfo.begin() + storageColumnInfoIndex);
                            --storageColumnInfoIndex;
                        }else{
                            
                            //  undefined type/types
                            notEqual = true;
                            break;
                        }
                    }else{
                        columnsToAdd.push_back(&storageColumnInfo);
                    }
                }
                return notEqual;
            }
            
            
        private:
            typedef storage_impl<Ts...> Super;
            typedef storage_impl<H, Ts...> Self;
        };
    }
}

#endif /* storage_impl_h */
