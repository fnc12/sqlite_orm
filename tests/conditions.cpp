#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("order by") {
    struct Object {
        int64 id;
        int n;
    };

    auto storage = make_storage(
        "",
        make_table("object", make_column("id", &Object::id, primary_key()), make_column("n", &Object::n)));
    storage.sync_schema();

    storage.insert<Object>({0, 1});
    storage.insert<Object>({0, 0});

    SECTION("column") {
        std::vector<int> expected{0, 1};
        // select n from object order by n
        auto rows = storage.select(&Object::n, order_by(&Object::n));
        REQUIRE(rows == expected);
    }
    SECTION("bindable") {
        std::vector<int> expected{1, 0};
        // select n from object order by ?
        // ? = 1
        auto rows = storage.select(&Object::n, order_by(1));
        REQUIRE(rows == expected);
    }
}
