#pragma once
#include <string>
#include "connection_holder.h"

namespace sqlite_orm {

    struct DbConnection {
        DbConnection(const std::string& filename) : m_filename(filename) {}

        const std::string m_filename;

        // internal::connection_ref& access_connection_ref() {
        //     return holds_connection;
        // }
        std::string filename() const {  // so we can access filename from make_storage(DbConnection,...)
            return m_filename;  // make_storage(DbConnection con,...) ==> calls make_storage(con.filename(), ....)
        }

        // internal::connection_ref get_connection() {
        //     internal::connection_ref res{ *this->connection };
        //     if (1 == this->connection->retain_count()) {
        //         // this->on_open_internal(this->connection->get()); // must be able to call it!
        //     }
        //     return res;
        // }
        // std::unique_ptr<internal::connection_holder> connection;
        internal::connection_ref* holds_connection = nullptr;  // make connection stay alive...
    };

}
