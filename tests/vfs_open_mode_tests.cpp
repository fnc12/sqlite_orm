#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdio>  //  std::remove
#if SQLITE_ORM_HAS_INCLUDE(<filesystem>)
#include <filesystem>
#endif
#include "catch_matchers.h"

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
    internal::serialize_result_type vfs_string = internal::vfs_object_to_string(vfs);

    connection_control options{true, vfs};
    auto storage = make_storage(":memory:", options, default_table);
    UNSCOPED_INFO("FAILED VFS: " << vfs_string);
    REQUIRE_NOTHROW(storage.open_forever());

    REQUIRE(storage.vfs() == vfs);
    REQUIRE(storage.open_flags() == open_mode::default_mode);

    SECTION("Storage copy operator carries over vfs option") {
        auto storage_copy = storage;
        CHECK(storage_copy.is_opened());
        CHECK(storage_copy.vfs() == vfs);
        CHECK(storage_copy.open_flags() == open_mode::default_mode);
    }
}

TEST_CASE("readwrite/readonly open modes behaves as expected") {
    const bool in_memory = GENERATE(true, false);
    const char* tmp_filename = in_memory ? ":memory:" : "open_mode.sqlite";

    connection_control options{true}, readonly_options{true};
    options.open_flags = open_mode::create_readwrite;
    readonly_options.open_flags = open_mode::readonly;

    if (!in_memory) {
        std::remove(tmp_filename);
    }

    SECTION("rw+ro") {
        auto storage = make_storage(tmp_filename, default_table, options);
        CHECK(storage.is_opened());
        REQUIRE(storage.open_flags() == open_mode::create_readwrite);
        REQUIRE_FALSE(storage.readonly());

        storage.sync_schema();
        const User dummy{"dummy"};
        storage.replace(dummy);

        SECTION("readonly open mode behaves as expected") {
            auto readonly_storage = make_storage(tmp_filename, readonly_options, default_table);
            CHECK(readonly_storage.is_opened());
            REQUIRE(readonly_storage.open_flags() == open_mode::readonly);
            REQUIRE(readonly_storage.readonly());

            if (in_memory) {
                SKIP("skipped for in-memory");
            }
            REQUIRE_NOTHROW(readonly_storage.sync_schema());
            CHECK_NOTHROW(readonly_storage.get<User>(dummy.id));
            const ErrorCodeExceptionMatcher readOnlyExceptionMatcher(sqlite_errc{SQLITE_READONLY});
            REQUIRE_THROWS_MATCHES(readonly_storage.remove<User>(dummy.id),
                                   std::system_error,
                                   readOnlyExceptionMatcher);
        }
    }
    SECTION("readonly fails with non-existing files") {
        if (in_memory) {
            SKIP();
        }

#if __cpp_lib_filesystem >= 201703L
        CHECK_FALSE(std::filesystem::exists(tmp_filename));
#endif
        const ErrorCodeExceptionMatcher cantOpenExceptionMatcher(sqlite_errc{SQLITE_CANTOPEN});
        REQUIRE_THROWS_MATCHES(make_storage(tmp_filename, readonly_options, default_table),
                               std::system_error,
                               cantOpenExceptionMatcher);
    }
}
#endif
