#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("drop table") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        std::string date;
    };
    const std::string usersTableName = "users";
    const std::string visitsTableName = "visits";
    auto storage = make_storage(
        {},
        make_table(usersTableName, make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
        make_table(visitsTableName, make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));
    REQUIRE(!storage.table_exists(usersTableName));
    REQUIRE(!storage.table_exists(visitsTableName));

    storage.sync_schema();
    REQUIRE(storage.table_exists(usersTableName));
    REQUIRE(storage.table_exists(visitsTableName));

    storage.drop_table(usersTableName);
    REQUIRE(!storage.table_exists(usersTableName));
    REQUIRE(storage.table_exists(visitsTableName));

    storage.drop_table(visitsTableName);
    REQUIRE(!storage.table_exists(usersTableName));
    REQUIRE(!storage.table_exists(visitsTableName));
}

TEST_CASE("rename table") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        std::string date;
    };
    const std::string usersTableName = "users";
    const std::string userNewTableName = "users_new";
    const std::string visitsTableName = "visits";
    const std::string visitsNewTableName = "visits_new";
    auto storage = make_storage(
        {},
        make_table(usersTableName, make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
        make_table(visitsTableName, make_column("id", &Visit::id, primary_key()), make_column("date", &Visit::date)));

    REQUIRE(!storage.table_exists(usersTableName));
    REQUIRE(!storage.table_exists(userNewTableName));
    REQUIRE(!storage.table_exists(visitsTableName));
    REQUIRE(!storage.table_exists(visitsNewTableName));
    REQUIRE(storage.tablename<User>() == usersTableName);
    REQUIRE(storage.tablename<User>() != userNewTableName);
    REQUIRE(storage.tablename<Visit>() == visitsTableName);
    REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

    storage.sync_schema();
    REQUIRE(storage.table_exists(usersTableName));
    REQUIRE(!storage.table_exists(userNewTableName));
    REQUIRE(storage.table_exists(visitsTableName));
    REQUIRE(!storage.table_exists(visitsNewTableName));
    REQUIRE(storage.tablename<User>() == usersTableName);
    REQUIRE(storage.tablename<User>() != userNewTableName);
    REQUIRE(storage.tablename<Visit>() == visitsTableName);
    REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

    SECTION("with 1 argument") {
        storage.rename_table<User>(userNewTableName);
        REQUIRE(storage.table_exists(usersTableName));
        REQUIRE(!storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE(!storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() != usersTableName);
        REQUIRE(storage.tablename<User>() == userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

        storage.rename_table<Visit>(visitsNewTableName);
        REQUIRE(storage.table_exists(usersTableName));
        REQUIRE(!storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE(!storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() != usersTableName);
        REQUIRE(storage.tablename<User>() == userNewTableName);
        REQUIRE(storage.tablename<Visit>() != visitsTableName);
        REQUIRE(storage.tablename<Visit>() == visitsNewTableName);
    }
    SECTION("with 2 arguments") {

        storage.rename_table(usersTableName, userNewTableName);
        REQUIRE(!storage.table_exists(usersTableName));
        REQUIRE(storage.table_exists(userNewTableName));
        REQUIRE(storage.table_exists(visitsTableName));
        REQUIRE(!storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() == usersTableName);
        REQUIRE(storage.tablename<User>() != userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);

        storage.rename_table(visitsTableName, visitsNewTableName);
        REQUIRE(!storage.table_exists(usersTableName));
        REQUIRE(storage.table_exists(userNewTableName));
        REQUIRE(!storage.table_exists(visitsTableName));
        REQUIRE(storage.table_exists(visitsNewTableName));
        REQUIRE(storage.tablename<User>() == usersTableName);
        REQUIRE(storage.tablename<User>() != userNewTableName);
        REQUIRE(storage.tablename<Visit>() == visitsTableName);
        REQUIRE(storage.tablename<Visit>() != visitsNewTableName);
    }
}
