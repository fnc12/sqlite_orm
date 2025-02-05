#pragma once

#include <sqlite3.h>

namespace sqlite_orm {

    enum class open_mode {
        default_mode = 0,
        create_readwrite = 0,
        readonly = 1,
    };
}

namespace sqlite_orm::internal {
    constexpr int open_mode_to_int_flags(sqlite_orm::open_mode open) {

        switch (open) {
            case open_mode::readonly:
                return SQLITE_OPEN_READONLY;
            case open_mode::create_readwrite:
                return SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
        };

        return -1;
    }
}
