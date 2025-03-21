#pragma once

#include "functional/config.h"
#include "serialize_result_type.h"

SQLITE_ORM_EXPORT namespace sqlite_orm {

#ifdef SQLITE_ORM_UNIX
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type unix_vfs_name = "unix";
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type unix_posix_vfs_name = unix_vfs_name;
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type unix_dotfile_vfs_name = "unix-dotfile";
#ifdef SQLITE_ORM_APPLE
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type unix_afp_vfs_name = "unix-afp";
#endif
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type default_vfs_name = "unix";
#endif

#ifdef SQLITE_ORM_WIN
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type win32_vfs_name = "win32";
    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type win32_longpath_vfs_name = "win32-longpath";

    SQLITE_ORM_INLINE_VAR constexpr internal::string_constant_type default_vfs_name = win32_vfs_name;
#endif
}
