#include <sqlite3.h>

#pragma once

namespace sqlite_orm {
    enum class open_mode_t {
        default_mode = 0,
        create_readwrite = 0,
        readonly = 1,
    };

    namespace internal {
        inline constexpr int to_int_flags(open_mode_t open) {

            switch (open) {
                case open_mode_t::readonly:
                    return SQLITE_OPEN_READONLY;
                default:
                case open_mode_t::create_readwrite:
                    return SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE;
            };
        }
    }

}
