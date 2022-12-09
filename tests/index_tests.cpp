#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("index") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    SECTION("simple id") {
        auto storage = make_storage({}, make_index("id_index", &User::id), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("asc") {
        auto storage = make_storage({}, make_index("id_index", indexed_column(&User::id).asc()), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("desc") {
        auto storage = make_storage({}, make_index("id_index", indexed_column(&User::id).desc()), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("simple name") {
        auto storage = make_storage({}, make_index("name_index", &User::name), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("explicit name") {
        auto storage = make_storage({}, make_index("name_index", indexed_column(&User::name)), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("collate") {
        auto storage = make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary")), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("collate asc") {
        auto storage =
            make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary").asc()), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
    SECTION("collate desc") {
        auto storage =
            make_storage({}, make_index("name_index", indexed_column(&User::name).collate("binary").desc()), table);
        REQUIRE_NOTHROW(storage.sync_schema());
    }
}

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("filtered index") {
    using Catch::Matchers::Contains;

    struct Test {
        std::optional<int> field1 = 0;
        std::optional<int> field2 = 0;
    };
    SECTION("1") {
        auto storage = make_storage(
            {},
            make_unique_index("ix2", &Test::field1, ifnull<int>(&Test::field2, 0)),
            make_table("test", make_column("Field1", &Test::field1), make_column("Field2", &Test::field2)));
        REQUIRE_NOTHROW(storage.sync_schema());

        storage.insert(Test{1, std::nullopt});
        REQUIRE_THROWS_WITH(storage.insert(Test{1, std::nullopt}), Contains("constraint failed"));
    }
    SECTION("2") {
        auto storage = make_storage(
            {},
            make_unique_index("ix2", &Test::field1, where(is_not_null(&Test::field1))),
            make_table("test", make_column("Field1", &Test::field1), make_column("Field2", &Test::field2)));
        REQUIRE_NOTHROW(storage.sync_schema());
    }
}
#endif  //  SQLITE_ORM_OPTIONAL_SUPPORTED

TEST_CASE("Escaped index name") {
    struct User {
        std::string group;
    };
    auto storage = make_storage("index_group.sqlite",
                                make_index("index", &User::group),
                                make_table("users", make_column("group", &User::group)));
    REQUIRE_NOTHROW(storage.sync_schema());
}

TEST_CASE("Compound index") {
    struct User {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{move(name)} {}
#endif
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));

    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    decltype(value) expected;

    auto index =
        make_index("idx_users_id_and_name", indexed_column(&User::id).asc(), indexed_column(&User::name).asc());
    value = internal::serialize(index, context);
    expected = R"(CREATE INDEX IF NOT EXISTS "idx_users_id_and_name" ON "users" ("id" ASC, "name" ASC))";
    REQUIRE(value == expected);
    auto storage = make_storage("compound_index.sqlite", index, table);
    REQUIRE_NOTHROW(storage.sync_schema());
    REQUIRE_NOTHROW(storage.insert(User{1, "juan"}));
}
