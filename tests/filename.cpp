#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("filename") {
    {
        auto storage = make_storage("");
        REQUIRE(storage.filename() == "");
    }
    {
        auto storage = make_storage(":memory:");
        REQUIRE(storage.filename() == ":memory:");
    }
    {
        auto storage = make_storage("myDatabase.sqlite");
        REQUIRE(storage.filename() == "myDatabase.sqlite");
    }
}
