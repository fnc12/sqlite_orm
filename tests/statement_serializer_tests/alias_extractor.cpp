#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;
using internal::alias_extractor;

TEST_CASE("alias extractor") {
    struct User {};

    SECTION("column alias") {
        REQUIRE(alias_extractor<colalias_a>::get() == "a");
    }
    SECTION("table") {
        REQUIRE(alias_extractor<User>::get() == "");
    }
    SECTION("table alias") {
        REQUIRE(alias_extractor<alias_a<User>>::get() == "a");
    }
}
