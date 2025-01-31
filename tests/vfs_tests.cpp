#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

struct User {
    std::string id;
};

using namespace sqlite_orm;

static const auto table = make_table("users", make_column("id", &User::id, primary_key()));

TEST_CASE("vfs modes open successfully") {

#ifdef SQLITE_ORM_MAC
    vfs_t vfs_enum = GENERATE(vfs_t::unix, vfs_t::unix_posix, vfs_t::unix_dotfile, vfs_t::unix_afp);
#elif defined(SQLITE_ORM_UNIX);
    vfs_t vfs_enum = GENERATE(vfs_t::unix, vfs_t::unix_posix, vfs_t::unix_dotfile);
#elif defined(SQLITE_ORM_WINDOWS)
    vfs_t vfs_enum = GENERATE(vfs_t::win32, vfs_t::win32_longpath);
#endif

    std::string vfs_string = internal::to_string(vfs_enum);
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    auto storage = make_storage_v2("", vfs_string, flags, table);
    storage.open_forever();

    INFO("VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
}

TEST_CASE("invalid vfs modes throw") {
    std::string vfs = GENERATE("posix", "unix-dot", "garbage_string");
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    INFO("VFS: " << vfs);
    REQUIRE_THROWS_AS(make_storage_v2("", vfs, flags, table), std::system_error);
}

TEST_CASE("invalid flags throw") {
    std::string vfs = "";
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_READONLY;

    INFO("VFS: " << vfs);
    REQUIRE_THROWS_AS(make_storage_v2("", vfs, flags, table), std::system_error);
}
