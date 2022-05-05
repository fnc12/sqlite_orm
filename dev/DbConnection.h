#pragma once
#include <string>
#include "connection_holder.h"

namespace sqlite_orm {

    struct DbConnection {
        DbConnection(const std::string& filename) :
            connection(std::make_unique<internal::connection_holder>(filename)), holds_connection(get_connection()) {}

        internal::connection_ref& access_connection_ref() {
            return holds_connection;
        }
        std::string filename() const {  // so we can access filename from make_storage(DbConnection,...)
            return connection->filename;
        }

      private:
        internal::connection_ref get_connection() {
            internal::connection_ref res{*this->connection};
            if(1 == this->connection->retain_count()) {
                // this->on_open_internal(this->connection->get());
            }
            return res;
        }
        std::unique_ptr<internal::connection_holder> connection;
        internal::connection_ref holds_connection;  // make connection stay alive...
    };

}
