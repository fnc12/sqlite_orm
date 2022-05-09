#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Is null") {
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, decltype(name) name = nullptr) : id{id}, name{move(name)} {}
#endif
    };
    auto storage = make_storage(
        "",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    REQUIRE(storage.count<User>() == 0);
    storage.replace(User{1, std::make_unique<std::string>("Sheldon")});
    REQUIRE(storage.count<User>() == 1);
    storage.replace(User{2});
    REQUIRE(storage.count<User>() == 2);
    storage.replace(User{3, std::make_unique<std::string>("Leonard")});
    REQUIRE(storage.count<User>() == 3);
    REQUIRE(storage.count<User>(where(is_null(&User::name))) == 1);
    REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 2);
}
