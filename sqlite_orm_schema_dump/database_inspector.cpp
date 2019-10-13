#include "database_inspector.h"
#include <stdexcept>

DatabaseInspector::DatabaseInspector(DbConnection connection)
    :m_connection(connection)
{
}

DatabaseInspector::Tables DatabaseInspector::readTablesFromDb(const std::vector<std::string>& tables)
{
    DatabaseInspector::Tables retval(new mstch::array);
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
            retval->push_back(mstch::map{{"name", tableName}, {"config", false}});
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
    for (auto tableNode : *tables) {
        auto table = boost::get<mstch::map>(tableNode);
        std::string tableName = boost::get<std::string>(table["name"]);
        mstch::array integer_columns;
        mstch::array text_columns;
        mstch::array blob_columns;
        mstch::array real_columns;
        mstch::array numeric_columns;

        sqlite3_stmt* stmt_handle;
        //there is no parameter binding available for pragma queries -> hence we do not bind the table name
        if(SQLITE_OK != sqlite3_prepare_v2(m_connection.get(), (std::string("pragma table_info(\"") + tableName + "\")").c_str(), -1, &stmt_handle, nullptr)) {
            throw std::runtime_error(std::string("can not get the tables from the database ") + sqlite3_errmsg(m_connection.get()));
        }
        auto stmt = std::shared_ptr<sqlite3_stmt>(stmt_handle, [](sqlite3_stmt* handle) { sqlite3_finalize(handle); });
        if(SQLITE_ROW == sqlite3_step(stmt.get())) {
            mstch::map rowDescription;
            std::string columnName = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1)));
            rowDescription["name"] = columnName;
            std::string rowType(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2)));
            rowDescription["notNull"] = (sqlite3_column_int(stmt.get(), 3) != 0);
            rowDescription["pl"] = (sqlite3_column_int(stmt.get(), 5) != 0);

            if(rowType == "INTEGER") {
                rowDescription["defaultValue"] = sqlite3_column_int(stmt.get(), 4);
                integer_columns.push_back(rowDescription);
            } else if(rowType == "TEXT") {
                rowDescription["defaultValue"] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 4)));;
                text_columns.push_back(rowDescription);
            } else if(rowType == "BLOB") {
                std::size_t byteCount = static_cast<std::size_t>(sqlite3_column_bytes(stmt.get(), 4));
                rowDescription["defaultValue"] = std::string(reinterpret_cast<const char*>(sqlite3_column_blob(stmt.get(), 4)), byteCount);
                blob_columns.push_back(rowDescription);
            } else if(rowType == "REAL") {
                rowDescription["defaultValue"] = sqlite3_column_double(stmt.get(), 4);
                real_columns.push_back(rowDescription);
            } else {
                throw std::runtime_error("unknown data type for column " + tableName + "." + columnName);
            }
        } else {
            throw std::runtime_error(std::string("error while reading the tables from the database ") + sqlite3_errmsg(m_connection.get()));
        }
        mstch::map tableDescription{{"integer_columns", integer_columns},{"text_columns", text_columns},
                                    {"blob_columns", blob_columns},{"real_columns", real_columns},
                                    {"numeric_columns", numeric_columns}};
        table["config"] = tableDescription;
    }
}
