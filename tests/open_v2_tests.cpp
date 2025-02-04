#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

struct User {
    std::string id;
};

using namespace sqlite_orm;

static const auto default_table = make_table("users", make_column("id", &User::id, primary_key()));

/******************************************************************************/
/******************************  STATIC CHECKS  *******************************/
/******************************************************************************/

TEST_CASE("vfs string conversion returns expected strings") {

#ifdef SQLITE_ORM_UNIX
    REQUIRE(internal::to_string(vfs_mode::unix) == "unix");
    REQUIRE(internal::to_string(vfs_mode::unix_posix) == "unix");
    REQUIRE(internal::to_string(vfs_mode::unix_dotfile) == "unix-dotfile");
#ifdef SQLITE_ORM_APPLE
    REQUIRE(internal::to_string(vfs_mode::unix_afp) == "unix-afp");
#endif
#endif

#ifdef SQLITE_ORM_WIN
    REQUIRE(internal::to_string(vfs_mode::win32) == "win32");
    REQUIRE(internal::to_string(vfs_mode::win32_longpath) == "win32-longpath");
#endif
}

TEST_CASE("open_mode flag conversion returns expected flags") {
    STATIC_REQUIRE(internal::to_int_flags(open_mode::default_mode) == (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::to_int_flags(open_mode::create_readwrite) == (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::to_int_flags(open_mode::readonly) == (SQLITE_OPEN_READONLY));
}

/******************************************************************************/
/*****************************  RUNTIME CHECKS  *******************************/
/******************************************************************************/

TEST_CASE("vfs modes open successfully") {

#if defined(SQLITE_ORM_APPLE)
    vfs_mode vfs_enum = GENERATE(vfs_mode::unix, vfs_mode::unix_posix, vfs_mode::unix_dotfile, vfs_mode::unix_afp);
#elif defined(SQLITE_ORM_UNIX)
    vfs_mode vfs_enum = GENERATE(vfs_mode::unix, vfs_mode::unix_posix, vfs_mode::unix_dotfile);
#elif defined(SQLITE_ORM_WIN)
    vfs_mode vfs_enum = GENERATE(vfs_mode::win32, vfs_mode::win32_longpath);
#endif

    auto storage = make_storage("", vfs_enum, open_mode::default_mode, default_table);
    REQUIRE_NOTHROW(storage.open_forever());

    std::string vfs_string = internal::to_string(vfs_enum);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
    REQUIRE(storage.vfs_mode() == vfs_enum);
}

TEST_CASE("create/readwrite open mode behaves as expected") {

    const bool memory = GENERATE(true, false);
    const char* tmp_filename = memory ? ":memory:" : "open_mode.sqlite";

    if (!memory) {
        std::remove(tmp_filename);
    }

    {
        auto storage = make_storage(tmp_filename, vfs_mode::default_vfs, open_mode::create_readwrite, default_table);

        CHECK_NOTHROW(storage.open_forever());

        CHECK(storage.is_opened());
        CHECK(storage.open_mode() == open_mode::create_readwrite);
        CHECK(!storage.readonly());

        SECTION("readonly open mode behaves as expected") {
            auto readonly_storage =
                make_storage(tmp_filename, vfs_mode::default_vfs, open_mode::readonly, default_table);
            CHECK_NOTHROW(readonly_storage.open_forever());

            CHECK(readonly_storage.is_opened());
            CHECK(readonly_storage.open_mode() == open_mode::readonly);
            CHECK(readonly_storage.readonly());
        }

        if (!memory) {
            REQUIRE(std::remove(tmp_filename) == 0);
        }
    }

    if (!memory) {
        SECTION("readonly fails with deleted files") {
            auto readonly_storage =
                make_storage(tmp_filename, vfs_mode::default_vfs, open_mode::readonly, default_table);
            REQUIRE_THROWS_AS(readonly_storage.open_forever(), std::system_error);
        }
    }
}
