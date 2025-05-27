#pragma once

#include "serialize_result_type.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

#ifdef SQLITE_ORM_UNIX
    inline constexpr internal::string_constant_type unix_vfs_name = "unix";
    inline constexpr internal::string_constant_type unix_posix_vfs_name = unix_vfs_name;
    inline constexpr internal::string_constant_type unix_dotfile_vfs_name = "unix-dotfile";
#ifdef SQLITE_ORM_APPLE
    inline constexpr internal::string_constant_type unix_afp_vfs_name = "unix-afp";
#endif
    inline constexpr internal::string_constant_type default_vfs_name = unix_vfs_name;
#endif

#ifdef SQLITE_ORM_WIN
    inline constexpr internal::string_constant_type win32_vfs_name = "win32";
    inline constexpr internal::string_constant_type win32_longpath_vfs_name = "win32-longpath";

    inline constexpr internal::string_constant_type default_vfs_name = win32_vfs_name;
#endif
}
