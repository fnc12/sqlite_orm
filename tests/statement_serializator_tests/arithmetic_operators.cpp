#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator arithmetic operators") {
    internal::serializator_context_base context;
    {
        auto value = serialize(add(3, 5), context);
        REQUIRE(value == "(3 + 5)");
    }
    {
        auto value = serialize(sub(5, -9), context);
        REQUIRE(value == "(5 - -9)");
    }
    {
        auto value = serialize(mul(10, 0.5), context);
        REQUIRE(value == "(10 * 0.5)");
    }
    {
        auto value = serialize(sqlite_orm::div(10, 2), context);
        REQUIRE(value == "(10 / 2)");
    }
    {
        auto value = serialize(mod(20, 3), context);
        REQUIRE(value == "(20 % 3)");
    }
}
