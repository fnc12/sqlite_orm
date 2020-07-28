#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator collate") {
    internal::serializator_context_base context;
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
