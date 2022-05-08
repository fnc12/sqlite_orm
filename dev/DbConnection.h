#pragma once
#include <string>
#include "connection_holder.h"

namespace sqlite_orm {

    struct DbConnection {
        DbConnection(const std::string& filename) :
            connection{std::make_shared<internal::connection_holder>(filename)} {
            this->connection->retain();  // opens the connection and stores the sqlite3*
        }

        DbConnection(const DbConnection& o) {
            this->connection = o.connection;
            this->connection->retain();
        }

        ~DbConnection() {
            this->connection->release();
        }
        std::string get_filename() const {  // so we can access filename from make_storage(DbConnection,...)
            return this->connection->filename;
        }
        std::shared_ptr<internal::connection_holder> connection;
        sqlite3* get() const {
            return connection->get();
        }
    };

}
