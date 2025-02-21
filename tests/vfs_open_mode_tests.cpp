#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_CTAD_SUPPORTED

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

    auto storage = make_storage(":memory:", connection_control{.vfs_mode = vfs}, default_table);
    storage.sync_schema();
    REQUIRE_NOTHROW(storage.open_forever());

    internal::serialize_result_type vfs_string = internal::vfs_object_to_string(vfs);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE(storage.is_opened());
    REQUIRE(storage.vfs() == vfs);
    REQUIRE(storage.open_flags() == open_mode::default_mode);
}

TEST_CASE("create/readwrite open mode behaves as expected") {

    const bool in_memory = GENERATE(true, false);
    const char* tmp_filename = in_memory ? ":memory:" : "open_mode.sqlite";

    connection_control options = {.open_flags = open_mode::create_readwrite};
    connection_control readonly_options = {.open_flags = open_mode::readonly};

    if (!in_memory) {
        std::remove(tmp_filename);
    }

    {
        auto storage = make_storage(tmp_filename, default_table, options);
        REQUIRE_NOTHROW(storage.sync_schema());

        CHECK_NOTHROW(storage.open_forever());

        CHECK(storage.is_opened());
        CHECK(storage.open_flags() == open_mode::create_readwrite);
        CHECK(!storage.readonly());

        User dummy{"dummy"};
        CHECK_NOTHROW(storage.replace(dummy));
        CHECK(storage.get_pointer<User>(dummy.id) != nullptr);

        SECTION("readonly open mode behaves as expected") {
            auto readonly_storage = make_storage(tmp_filename, readonly_options, default_table);
            REQUIRE_NOTHROW(readonly_storage.open_forever());

            CHECK(readonly_storage.is_opened());
            CHECK(readonly_storage.open_flags() == open_mode::readonly);
            CHECK(readonly_storage.readonly());
            CHECK_THROWS_AS(readonly_storage.replace(dummy), std::system_error);
            CHECK(storage.get_pointer<User>(dummy.id) != nullptr);
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

#endif
