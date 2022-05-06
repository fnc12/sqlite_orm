#pragma once
#include <string>
#include "connection_holder.h"

namespace sqlite_orm {

    struct DbConnection {
        DbConnection(const std::string& filename) :
            connection(std::make_shared<internal::connection_holder>(filename)) {
            this->connection->retain();
            db = this->connection->get();
        }

        ~DbConnection() {
            this->connection->release();
        }
        std::string get_filename() const {  // so we can access filename from make_storage(DbConnection,...)
            return this->connection->filename;
        }

        std::shared_ptr<internal::connection_holder> connection;
        sqlite3* db = nullptr;
    };

}
