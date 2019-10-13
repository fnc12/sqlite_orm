#ifndef DATABASE_INSPECTOR_H
#define DATABASE_INSPECTOR_H

#include <mstch/mstch.hpp>
#include <memory>
#include <sqlite3.h>

class DatabaseInspector
{
public:
    typedef std::shared_ptr<sqlite3> DbConnection;
    typedef std::shared_ptr<mstch::array> Tables;

    DatabaseInspector(DbConnection connection);

    Tables readTablesFromDb();
    void readTableDetailsFromDb(Tables tables);

private:
    DbConnection m_connection;
};

#endif // DATABASE_INSPECTOR_H
