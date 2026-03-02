#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Is null") {
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    auto storage = make_storage(
        "",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    REQUIRE(storage.count<User>() == 0);
    storage.replace(User{1, std::make_unique<std::string>("Sheldon")});
    storage.replace(User{2});
    storage.replace(User{3, std::make_unique<std::string>("Leonard")});
    REQUIRE(storage.count<User>() == 3);

    SECTION("is_null in where clause") {
        REQUIRE(storage.count<User>(where(is_null(&User::name))) == 1);
    }

    SECTION("is_not_null in where clause") {
        REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 2);
    }

    SECTION("select is_null as expression") {
        auto rows = storage.select(is_null(&User::name));
        REQUIRE(rows.size() == 3);
        REQUIRE(rows[0] == false);
        REQUIRE(rows[1] == true);
        REQUIRE(rows[2] == false);
    }

    SECTION("select is_not_null as expression") {
        auto rows = storage.select(is_not_null(&User::name));
        REQUIRE(rows.size() == 3);
        REQUIRE(rows[0] == true);
        REQUIRE(rows[1] == false);
        REQUIRE(rows[2] == true);
    }

    SECTION("negated is_null in where clause") {
        auto rows = storage.select(&User::id, where(!is_null(&User::name)));
        REQUIRE(rows.size() == 2);
        REQUIRE(rows[0] == 1);
        REQUIRE(rows[1] == 3);
    }

    SECTION("negated is_not_null in where clause") {
        auto rows = storage.select(&User::id, where(!is_not_null(&User::name)));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows[0] == 2);
    }
}
