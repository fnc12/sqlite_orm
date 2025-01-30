#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

struct User {
    std::string id;
};

using namespace sqlite_orm;

static const auto default_table = make_table("users", make_column("id", &User::id, primary_key()));

TEST_CASE("vfs modes open successfully") {

#if defined(SQLITE_ORM_APPLE)
    vfs_object vfs = GENERATE(vfs_object::unix, vfs_object::unix_posix, vfs_object::unix_dotfile, vfs_object::unix_afp);
#elif defined(SQLITE_ORM_UNIX)
    vfs_object vfs = GENERATE(vfs_object::unix, vfs_object::unix_posix, vfs_object::unix_dotfile);
#elif defined(SQLITE_ORM_WIN)
    vfs_object vfs = GENERATE(vfs_object::win32, vfs_object::win32_longpath);
#endif

    storage_options options;
    options.vfs_option = vfs;

    auto storage = make_storage(":memory:", options, default_table);
    REQUIRE_NOTHROW(storage.open_forever());

    internal::serialize_result_type vfs_string = internal::vfs_object_to_string(options.vfs_option);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
    REQUIRE(storage.vfs_object() == options.vfs_option);
    REQUIRE(storage.open_mode() == options.open_option);
}

TEST_CASE("create/readwrite open mode behaves as expected") {

    const bool in_memory = GENERATE(true, false);
    const char* tmp_filename = in_memory ? ":memory:" : "open_mode.sqlite";

    storage_options options, readonly_options;
    options.open_option = open_mode::create_readwrite;
    readonly_options.open_option = open_mode::readonly;

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
