#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer collate") {
    internal::schema_objects<> storage;
    internal::serializer_context<internal::schema_objects<>> context{storage};
    {
        auto col = collate_nocase();
        auto value = serialize(col, context);
        REQUIRE(value == "COLLATE NOCASE");
    }
    {
        auto col = collate_binary();
        auto value = serialize(col, context);
        REQUIRE(value == "COLLATE BINARY");
    }
    {
        auto col = collate_rtrim();
        auto value = serialize(col, context);
        REQUIRE(value == "COLLATE RTRIM");
    }
}
