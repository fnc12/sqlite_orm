#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;
using internal::alias_extractor;

TEST_CASE("alias extractor") {
    struct User {};

    SECTION("column alias") {
        REQUIRE(alias_extractor<colalias_a>::extract() == "a");
        REQUIRE(alias_extractor<colalias_a>::as_alias() == "a");
    }
    SECTION("table") {
        REQUIRE(alias_extractor<User>::extract() == "");
        REQUIRE(alias_extractor<User>::as_alias() == "");
    }
    SECTION("table alias") {
        REQUIRE(alias_extractor<alias_a<User>>::extract() == "a");
        REQUIRE(alias_extractor<alias_a<User>>::as_alias() == "a");
    }
}
