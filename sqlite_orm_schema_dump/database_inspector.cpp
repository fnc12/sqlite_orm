#include "database_inspector.h"

DatabaseInspector::DatabaseInspector(DbConnection connection)
    :m_connection(connection)
{

}

DatabaseInspector::Tables DatabaseInspector::readTablesFromDb()
{
    DatabaseInspector::Tables retval(new mstch::array);
    return retval;
}

void DatabaseInspector::readTableDetailsFromDb(DatabaseInspector::Tables tables)
{

}
