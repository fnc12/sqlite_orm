#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Not operator") {
    struct Object {
        int id = 0;
    };

    auto storage = make_storage("", make_table("objects", make_column("id", &Object::id, primary_key())));
    storage.sync_schema();

    storage.replace(Object{2});
    storage.replace(Object{3});

    std::vector<int> rows;
    std::vector<int> expected;
    SECTION("is_equal") {
        rows = storage.select(&Object::id, where(not is_equal(&Object::id, 1)));
        expected.push_back(2);
        expected.push_back(3);
    }
    SECTION("is_not_equal") {
        rows = storage.select(&Object::id, where(not is_not_equal(&Object::id, 3)));
        expected.push_back(3);
    }
    SECTION("greater_than") {
        rows = storage.select(&Object::id, where(not greater_than(&Object::id, 2)));
        expected.push_back(2);
    }
    SECTION("greater_or_equal") {
        rows = storage.select(&Object::id, where(not greater_or_equal(&Object::id, 3)));
        expected.push_back(2);
    }
    SECTION("less_than") {
        rows = storage.select(&Object::id, where(not less_than(&Object::id, 3)));
        expected.push_back(3);
    }
    SECTION("less_or_equal") {
        rows = storage.select(&Object::id, where(not less_or_equal(&Object::id, 2)));
        expected.push_back(3);
    }
    SECTION("in") {
        rows = storage.select(&Object::id, where(not in(&Object::id, {1, 2})));
        expected.push_back(3);
    }
    SECTION("is_null") {
        rows = storage.select(&Object::id, where(not is_null(&Object::id)));
        expected.push_back(2);
        expected.push_back(3);
    }
    SECTION("is_not_null") {
        rows = storage.select(&Object::id, where(not is_not_null(&Object::id)));
    }
    SECTION("like") {
        rows = storage.select(&Object::id, where(not like(cast<std::string>(&Object::id), "2")));
        expected.push_back(3);
    }
    SECTION("glob") {
        rows = storage.select(&Object::id, where(not like(cast<std::string>(&Object::id), "3")));
        expected.push_back(2);
    }
    SECTION("exists") {
        rows = storage.select(&Object::id, where(not exists(select(&Object::id, where(is_equal(&Object::id, 2))))));
    }
    REQUIRE(rows == expected);
}
