#pragma once
#include <string>
#include "connection_holder.h"

namespace sqlite_orm {

    struct DbConnection {
        DbConnection(const std::string& filename) :
            connection(std::make_unique<internal::connection_holder>(filename)) {
            this->connection->retain();
        }

        ~DbConnection() {
            this->connection->release();
        }
        std::string get_filename() const {  // so we can access filename from make_storage(DbConnection,...)
            return this->connection->filename;
        }

        std::unique_ptr<internal::connection_holder> connection;
    };

}
