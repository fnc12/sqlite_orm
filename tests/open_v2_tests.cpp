#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define SQLITE_ORM_UNIX 1
#elif _WIN32
#define SQLITE_ORM_WINDOWS 1
#else
#error "Unsupported Operating System"
#endif

struct User {
    std::string id;
};

using namespace sqlite_orm;

static const auto table = make_table("users", make_column("id", &User::id, primary_key()));

TEST_CASE("vfs modes open successfully") {

#if SQLITE_ORM_UNIX
    std::string vfs = GENERATE("", "unix", "unix-excl", "unix-dotfile", "unix-none", "unix-namedsem");
#elif SQLITE_ORM_WINDOWS
    std::string vfs = GENERATE("", "win32", "win32-longpath", "win32-none");
#endif

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    auto storage = make_storage("", table);
    storage.open_forever();

    INFO("VFS: " << vfs);
    REQUIRE(storage.is_opened());
}

TEST_CASE("invalid vfs modes throw") {
    std::string vfs = GENERATE("posix", "unix-dot", "garbage_string");
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    INFO("VFS: " << vfs);
    REQUIRE_THROWS_AS(make_storage("", vfs, flags, table), std::system_error);
}

TEST_CASE("invalid flags throw") {
    std::string vfs = "";
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_READONLY;

    INFO("VFS: " << vfs);
    REQUIRE_THROWS_AS(make_storage("", vfs, flags, table), std::system_error);
}
