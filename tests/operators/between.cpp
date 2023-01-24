#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Between") {
    struct Object {
        int id = 0;
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
    auto rows = storage.select(&Object::id, where(between(&Object::id, 1, 3)));
    REQUIRE(rows.size() == 3);
}
