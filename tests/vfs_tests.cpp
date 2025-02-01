#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

struct User {
    std::string id;
};

using namespace sqlite_orm;

static const auto table = make_table("users", make_column("id", &User::id, primary_key()));

TEST_CASE("vfs string conversion returns expected strings") {

#ifdef SQLITE_ORM_UNIX
    REQUIRE(internal::to_string(vfs_t::unix) == "unix");
    REQUIRE(internal::to_string(vfs_t::unix_posix) == "unix");
    REQUIRE(internal::to_string(vfs_t::unix_dotfile) == "unix-dotfile");
#ifdef SQLITE_ORM_MAC
    REQUIRE(internal::to_string(vfs_t::unix_afp) == "unix-afp");
#endif
#endif

#ifdef SQLITE_ORM_WIN
    REQUIRE(internal::to_string(vfs_t::win32) == "win32");
    REQUIRE(internal::to_string(vfs_t::win32_longpath) == "win32-longpath");
#endif
}

TEST_CASE("vfs modes open successfully") {

#ifdef SQLITE_ORM_MAC
    vfs_t vfs_enum = GENERATE(vfs_t::unix, vfs_t::unix_posix, vfs_t::unix_dotfile, vfs_t::unix_afp);
#elif defined(SQLITE_ORM_UNIX);
    vfs_t vfs_enum = GENERATE(vfs_t::unix, vfs_t::unix_posix, vfs_t::unix_dotfile);
#elif defined(SQLITE_ORM_WINDOWS)
    vfs_t vfs_enum = GENERATE(vfs_t::win32, vfs_t::win32_longpath);
#endif

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    auto storage = make_storage_v2("", vfs_enum, flags, table);
    storage.open_forever();

    std::string vfs_string = internal::to_string(vfs_enum);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
    REQUIRE(storage.vfs_type() == vfs_enum);
}
