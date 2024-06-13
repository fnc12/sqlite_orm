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
        REQUIRE_THAT(schemaRows, Equals(masterRows));

        auto schemaRows2 = storage.get_all<sqlite_master_table>();

        STATIC_REQUIRE(std::is_same_v<decltype(masterRows), decltype(schemaRows2)>);
        REQUIRE_THAT(schemaRows2, Equals(masterRows));

#if __cpp_lib_containers_ranges >= 202202L
        std::vector<sqlite_master> schemaRows3{std::from_range, storage.iterate<sqlite_master_table>()};
#else
        auto view = storage.iterate<sqlite_master_table>();
        std::vector<sqlite_master> schemaRows3{view.begin(), view.end()};
#endif
        REQUIRE_THAT(schemaRows2, Equals(masterRows));
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
