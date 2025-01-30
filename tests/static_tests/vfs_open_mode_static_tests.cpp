#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("vfs string conversion returns expected strings") {

#ifdef SQLITE_ORM_UNIX
    REQUIRE(internal::vfs_object_to_string(vfs_object::unix) == "unix");
    REQUIRE(internal::vfs_object_to_string(vfs_object::unix_posix) == "unix");
    REQUIRE(internal::vfs_object_to_string(vfs_object::unix_dotfile) == "unix-dotfile");
#ifdef SQLITE_ORM_APPLE
    REQUIRE(internal::vfs_object_to_string(vfs_object::unix_afp) == "unix-afp");
#endif
#endif

#ifdef SQLITE_ORM_WIN
    REQUIRE(internal::vfs_object_to_string(vfs_object::win32) == "win32");
    REQUIRE(internal::vfs_object_to_string(vfs_object::win32_longpath) == "win32-longpath");
#endif
}

TEST_CASE("open_mode flag conversion returns expected flags") {
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::default_mode) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::create_readwrite) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::readonly) == (SQLITE_OPEN_READONLY));
}
