#include <sqlite3.h>
#include "functional/config.h"

#pragma once

namespace sqlite_orm {

    enum class vfs {

#if defined(SQLITE_ORM_MAC) || defined(SQLITE_ORM_LINUX)

        unix = 0,
        unix_posix = 0,
        unix_dotfile = 1

#ifdef SQLITE_ORM_MAC
        ,
        unix_afp = 2,

#elif defined(SQLITE_ORM_LINUX)

#endif

#endif

#ifdef SQLITE_ORM_WIN
        win32 = 0,
        win32_longpath = 1,
#endif

    };

    namespace internal {
        inline const std::string& to_string(vfs v) {
            static std::string res[] = {
#if defined(SQLITE_ORM_MAC) || defined(SQLITE_ORM_LINUX)

                "unix",
                "unix-dotfile"

#ifdef SQLITE_ORM_MAC
                ,
                "unix-afp",

#elif defined(SQLITE_ORM_LINUX)

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
