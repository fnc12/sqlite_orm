#pragma once

#include <sqlite3.h>
#include "functional/config.h"
#include "serialize_result_type.h"

namespace sqlite_orm {

    enum class vfs_object {

        default_vfs = 0,
#ifdef SQLITE_ORM_UNIX
        unix = 0,
        unix_posix = 0,
        unix_dotfile = 1,
#ifdef SQLITE_ORM_APPLE
        unix_afp = 2,
#endif
#endif

#ifdef SQLITE_ORM_WIN
        win32 = 0,
        win32_longpath = 1,
#endif
        num_vfs_objects

    };

}

namespace sqlite_orm {
    namespace internal {

        inline const serialize_result_type& vfs_object_to_string(vfs_object v) {
            static constexpr size_t num_vfs_objects = static_cast<size_t>(vfs_object::num_vfs_objects);
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
            static const std::array<serialize_result_type, num_vfs_objects> idx2str = {
#else
            static const std::array<serialize_result_type, num_vfs_objects> idx2str = {
#endif

#ifdef SQLITE_ORM_UNIX
                "unix",
                "unix-dotfile",
#ifdef SQLITE_ORM_APPLE
                "unix-afp",
#endif
#endif

#ifdef SQLITE_ORM_WIN
                "win32",
                "win32-longpath",
#endif
            };

            return idx2str.at(static_cast<size_t>(v));
        }
    }
}
