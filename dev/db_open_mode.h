#pragma once

namespace sqlite_orm {

    enum class db_open_mode {
        default_ = 0,
        create_readwrite = 0,
        readonly = 1,
    };
}

namespace sqlite_orm {
    namespace internal {
        constexpr int db_open_mode_to_int_flags(db_open_mode open) {

            switch (open) {
                case db_open_mode::readonly:
                    return SQLITE_OPEN_READONLY;
                case db_open_mode::create_readwrite:
                    return SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
            };

            return -1;
        }
    }
}
