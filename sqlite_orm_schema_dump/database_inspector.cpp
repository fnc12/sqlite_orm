#include "database_inspector.h"
#include <stdexcept>

DatabaseInspector::DatabaseInspector(DbConnection connection)
    :m_connection(connection)
{
}

DatabaseInspector::Tables DatabaseInspector::readTablesFromDb(const std::vector<std::string>& tables)
{
    DatabaseInspector::Tables retval(new nlohmann::json);
    sqlite3_stmt* stmt_handle;
    if(SQLITE_OK != sqlite3_prepare_v2(m_connection.get(), "SELECT name FROM sqlite_master WHERE type='table';", -1, &stmt_handle, nullptr)) {
        throw std::runtime_error(std::string("can not get the tables from the database ") + sqlite3_errmsg(m_connection.get()));
    }
    auto stmt = std::shared_ptr<sqlite3_stmt>(stmt_handle, [](sqlite3_stmt* handle) { sqlite3_finalize(handle); });
    while(int err = sqlite3_step(stmt.get())) {
        if(err == SQLITE_ROW) {
            std::string tableName(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 0)));
            if(tables.size()) {
                auto it = std::find(tables.begin(), tables.end(), tableName);
                if(it == tables.end()) {
                    continue;
                }
            }
            retval->push_back(nlohmann::json{{"name", tableName}, {"columns", nlohmann::json::array()}});
            continue;
        } else if (err == SQLITE_DONE) {
            break;
        }
        throw std::runtime_error(std::string("error while reading the tables from the database ") + sqlite3_errmsg(m_connection.get()));
    }
    if(tables.size() && tables.size() != retval->size()) {
        throw std::runtime_error("not all requested tables are found in the database");
    }
    return retval;
}

void DatabaseInspector::readTableDetailsFromDb(DatabaseInspector::Tables tables)
{
    for (auto& table : *tables) {
        std::string tableName = table["name"];
        auto columns = nlohmann::json::array();

        sqlite3_stmt* stmt_handle;
        //there is no parameter binding available for pragma queries -> hence we do not bind the table name
        if(SQLITE_OK != sqlite3_prepare_v2(m_connection.get(), (std::string("pragma table_info(\"") + tableName + "\")").c_str(), -1, &stmt_handle, nullptr)) {
            throw std::runtime_error(std::string("can not get the tables from the database ") + sqlite3_errmsg(m_connection.get()));
        }
        auto stmt = std::shared_ptr<sqlite3_stmt>(stmt_handle, [](sqlite3_stmt* handle) { sqlite3_finalize(handle); });
        if(SQLITE_ROW == sqlite3_step(stmt.get())) {
            auto colDescription = nlohmann::json::object();
            std::string columnName = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1)));
            colDescription["name"] = columnName;
            std::string rowType(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
            colDescription["notNull"] = (sqlite3_column_int(stmt.get(), 3) != 0);
            colDescription["pl"] = (sqlite3_column_int(stmt.get(), 5) != 0);
            colDescription["type"] = rowType;
            if(rowType == "INTEGER") {
                colDescription["defaultValue"] = sqlite3_column_int(stmt.get(), 4);
            } else if(rowType == "TEXT") {
                colDescription["defaultValue"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4)));;
            } else if(rowType == "BLOB") {
                std::size_t byteCount = static_cast<std::size_t>(sqlite3_column_bytes(stmt.get(), 4));
                colDescription["defaultValue"] = std::string(reinterpret_cast<const char*>(sqlite3_column_blob(stmt.get(), 4)), byteCount);
            } else if(rowType == "REAL") {
                colDescription["defaultValue"] = sqlite3_column_double(stmt.get(), 4);
            } else {
                throw std::runtime_error("unknown data type for column " + tableName + "." + columnName);
            }
            columns.push_back(colDescription);
        } else {
            throw std::runtime_error(std::string("error while reading the tables from the database ") + sqlite3_errmsg(m_connection.get()));
        }
        table["columns"] = columns;
    }
}
