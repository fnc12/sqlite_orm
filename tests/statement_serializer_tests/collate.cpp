#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer collate") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("COLLATE NOCASE") {
        auto col = collate_nocase();
        value = serialize(col, context);
        expected = "COLLATE NOCASE";
    }
    SECTION("COLLATE BINARY") {
        auto col = collate_binary();
        value = serialize(col, context);
        expected = "COLLATE BINARY";
    }
    SECTION("COLLATE RTRIM") {
        auto col = collate_rtrim();
        value = serialize(col, context);
        expected = "COLLATE RTRIM";
    }
    REQUIRE(value == expected);
}
