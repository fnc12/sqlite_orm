#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Not operator") {
    struct Object {
        int id = 0;
    };

    auto storage = make_storage("", make_table("objects", make_column("id", &Object::id, primary_key())));
    storage.sync_schema();

    storage.replace(Object{2});

    auto rows = storage.select(&Object::id, where(not is_equal(&Object::id, 1)));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 2);
}
