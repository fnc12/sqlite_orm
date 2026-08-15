#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Between") {
    struct Object {
        int64 id = 0;
    };

    auto storage =
        make_storage("", make_table("objects", make_column("id", &Object::id, primary_key().autoincrement())));
    storage.sync_schema();

    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});

    auto allObjects = storage.get_all<Object>();
    REQUIRE(allObjects.size() == 5);

    SECTION("between in where clause") {
        auto rows = storage.select(&Object::id, where(between(&Object::id, 1, 3)));
        REQUIRE(rows.size() == 3);
    }

    SECTION("between as select expression") {
        auto rows = storage.select(between(&Object::id, 2, 4));
        REQUIRE(rows.size() == 5);
        REQUIRE(rows[0] == false);
        REQUIRE(rows[1] == true);
        REQUIRE(rows[2] == true);
        REQUIRE(rows[3] == true);
        REQUIRE(rows[4] == false);
    }

    SECTION("negated between in where clause") {
        auto rows = storage.select(&Object::id, where(!between(&Object::id, 2, 4)));
        REQUIRE(rows.size() == 2);
        REQUIRE(rows[0] == 1);
        REQUIRE(rows[1] == 5);
    }

    SECTION("between with empty result") {
        auto rows = storage.select(&Object::id, where(between(&Object::id, 10, 20)));
        REQUIRE(rows.empty());
    }

    SECTION("between with single match") {
        auto rows = storage.select(&Object::id, where(between(&Object::id, 3, 3)));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0] == 3);
    }
}
