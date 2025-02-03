#include <sqlite3.h>
#include "functional/config.h"

#pragma once

namespace sqlite_orm {

    enum class vfs_mode {

        default_vfs = 0,
#ifdef SQLITE_ORM_UNIX
        unix = 0,
        unix_posix = 0,
        unix_dotfile = 1,
#ifdef SQLITE_ORM_MAC
        unix_afp = 2,
#endif
#endif

#ifdef SQLITE_ORM_WIN
        win32 = 0,
        win32_longpath = 1,
#endif

    };

    namespace internal {
        inline const std::string& to_string(vfs_mode v) {
            static std::string res[] = {
#ifdef SQLITE_ORM_UNIX
                "unix",
                "unix-dotfile",
#ifdef SQLITE_ORM_MAC
                "unix-afp",
#endif
#endif

#ifdef SQLITE_ORM_WIN
                "win32",
                "win32-longpath",
#endif
            };

            return res[static_cast<size_t>(v)];
        }
    }

}
