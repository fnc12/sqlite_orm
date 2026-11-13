#pragma once

/** @file
 *  Purpose: name the built-in VFSes.
 *
 *  Intention: placed by `docs/internals/source-layout.md` (https://www.sqlite.org/vfs.html); read by
 *  `execution/storage_options.h` and `execution/connection_holder.h`, which open a database with the VFS a storage
 *  names.
 */

#ifndef SQLITE_ORM_IMPORT_STD_MODULE
#include <string_view>  //  std::string_view
#endif

SQLITE_ORM_EXPORT namespace sqlite_orm {

#ifdef SQLITE_ORM_UNIX
    /**
     *  https://www.sqlite.org/vfs.html#standard_unix_vfses
     */
    inline constexpr std::string_view unix_vfs_name = "unix";
    /**
     *  https://www.sqlite.org/vfs.html#standard_unix_vfses
     */
    inline constexpr std::string_view unix_posix_vfs_name = unix_vfs_name;
    /**
     *  https://www.sqlite.org/vfs.html#standard_unix_vfses
     */
    inline constexpr std::string_view unix_dotfile_vfs_name = "unix-dotfile";
#ifdef SQLITE_ORM_APPLE
    inline constexpr std::string_view unix_afp_vfs_name = "unix-afp";
#endif
    /**
     *  https://www.sqlite.org/vfs.html#specifying_which_vfs_to_use
     */
    inline constexpr std::string_view default_vfs_name = unix_vfs_name;
#endif

#ifdef SQLITE_ORM_WIN
    /**
     *  https://www.sqlite.org/vfs.html#standard_windows_vfses
     */
    inline constexpr std::string_view win32_vfs_name = "win32";
    /**
     *  https://www.sqlite.org/vfs.html#standard_windows_vfses
     */
    inline constexpr std::string_view win32_longpath_vfs_name = "win32-longpath";

    /**
     *  https://www.sqlite.org/vfs.html#specifying_which_vfs_to_use
     */
    inline constexpr std::string_view default_vfs_name = win32_vfs_name;
#endif
}
