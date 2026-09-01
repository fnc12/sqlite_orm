#pragma once

#include <sqlite3.h>

#if SQLITE_VERSION_NUMBER >= 3034000
SQLITE_ORM_EXPORT namespace sqlite_orm {

    /**
     *  Transaction state of a database connection, as reported by `sqlite3_txn_state()`.
     */
    enum class transaction_state {
        /**
         *  no transaction is currently pending
         */
        none = SQLITE_TXN_NONE,

        /**
         *  currently executing a read transaction
         */
        read = SQLITE_TXN_READ,

        /**
         *  currently executing a write transaction
         */
        write = SQLITE_TXN_WRITE,
    };
}
#endif
