#pragma once

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

#ifdef SQLITE_ORM_UNIX
    inline constexpr std::string_view unix_vfs_name = "unix";
    inline constexpr std::string_view unix_posix_vfs_name = unix_vfs_name;
    inline constexpr std::string_view unix_dotfile_vfs_name = "unix-dotfile";
#ifdef SQLITE_ORM_APPLE
    inline constexpr std::string_view unix_afp_vfs_name = "unix-afp";
#endif
    inline constexpr std::string_view default_vfs_name = unix_vfs_name;
#endif

#ifdef SQLITE_ORM_WIN
    inline constexpr std::string_view win32_vfs_name = "win32";
    inline constexpr std::string_view win32_longpath_vfs_name = "win32-longpath";

    inline constexpr std::string_view default_vfs_name = win32_vfs_name;
#endif
}
