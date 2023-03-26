#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using Catch::Matchers::Equals;

TEST_CASE("builtin tables") {
    SECTION("sqlite_schema") {
        auto storage = make_storage("", make_sqlite_schema_table());
        storage.sync_schema();

        auto masterRows = storage.get_all<sqlite_master>();
        std::ignore = masterRows;
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        auto schemaRows = storage.get_all<sqlite_schema>();

        STATIC_REQUIRE(std::is_same_v<decltype(masterRows), decltype(schemaRows)>);
#endif
    }

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
    SECTION("dbstat") {
        struct User {
            int id = 0;
            std::string name;
        };
        auto storage = make_storage(
            "",
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_dbstat_table());
        storage.sync_schema();

        storage.remove_all<User>();

        storage.replace(User{1, "Dua Lipa"});

        auto dbstatRows = storage.get_all<dbstat>();
        std::ignore = dbstatRows;
    }
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
}
