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
}
