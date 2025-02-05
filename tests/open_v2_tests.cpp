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
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::unix) == "unix");
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::unix_posix) == "unix");
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::unix_dotfile) == "unix-dotfile");
#ifdef SQLITE_ORM_APPLE
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::unix_afp) == "unix-afp");
#endif
#endif

#ifdef SQLITE_ORM_WIN
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::win32) == "win32");
    REQUIRE(internal::vfs_mode_to_string(vfs_mode::win32_longpath) == "win32-longpath");
#endif
}

TEST_CASE("open_mode flag conversion returns expected flags") {
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::default_mode) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::create_readwrite) ==
                   (SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE));
    STATIC_REQUIRE(internal::open_mode_to_int_flags(open_mode::readonly) == (SQLITE_OPEN_READONLY));
}

/******************************************************************************/
/*****************************  RUNTIME CHECKS  *******************************/
/******************************************************************************/

TEST_CASE("vfs modes open successfully") {

#if defined(SQLITE_ORM_APPLE)
    vfs_mode vfs = GENERATE(vfs_mode::unix, vfs_mode::unix_posix, vfs_mode::unix_dotfile, vfs_mode::unix_afp);
#elif defined(SQLITE_ORM_UNIX)
    vfs_mode vfs = GENERATE(vfs_mode::unix, vfs_mode::unix_posix, vfs_mode::unix_dotfile);
#elif defined(SQLITE_ORM_WIN)
    vfs_mode vfs = GENERATE(vfs_mode::win32, vfs_mode::win32_longpath);
#endif

    storage_options options;
    options.vfs_mode = vfs;

    auto storage = make_storage(":memory:", options, default_table);
    REQUIRE_NOTHROW(storage.open_forever());

    internal::serialize_result_type vfs_string = internal::vfs_mode_to_string(options.vfs_mode);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
    REQUIRE(storage.vfs_mode() == options.vfs_mode);
    REQUIRE(storage.open_mode() == options.open_mode);
}

TEST_CASE("create/readwrite open mode behaves as expected") {

    const bool in_memory = GENERATE(true, false);
    const char* tmp_filename = in_memory ? ":memory:" : "open_mode.sqlite";

    storage_options options, readonly_options;
    options.open_mode = open_mode::create_readwrite;
    readonly_options.open_mode = open_mode::readonly;

    if (!in_memory) {
        std::remove(tmp_filename);
    }

    {
        auto storage = make_storage(tmp_filename, options, default_table);

        CHECK_NOTHROW(storage.open_forever());

        CHECK(storage.is_opened());
        CHECK(storage.open_mode() == open_mode::create_readwrite);
        CHECK(!storage.readonly());

        SECTION("readonly open mode behaves as expected") {
            auto readonly_storage = make_storage(tmp_filename, readonly_options, default_table);
            CHECK_NOTHROW(readonly_storage.open_forever());

            CHECK(readonly_storage.is_opened());
            CHECK(readonly_storage.open_mode() == open_mode::readonly);
            CHECK(readonly_storage.readonly());
        }
    }

    if (!in_memory) {
        INFO(tmp_filename);
        REQUIRE(std::remove(tmp_filename) == 0);
    }

    if (!in_memory) {
        SECTION("readonly fails with deleted files") {
            auto readonly_storage = make_storage(tmp_filename, readonly_options, default_table);
            REQUIRE_THROWS_AS(readonly_storage.open_forever(), std::system_error);
        }
    }
}
