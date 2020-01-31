#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator comparison operators") {
    internal::serializator_context_base context;
    {
        auto value = serialize(lesser_than(4, 5), context);
        REQUIRE(value == "(4 < 5)");
    }
    {
        auto value = serialize(lesser_or_equal(10, 15), context);
        REQUIRE(value == "(10 <= 15)");
    }
    {
        auto value = serialize(greater_than(1, 0.5), context);
        REQUIRE(value == "(1 > 0.5)");
    }
    {
        auto value = serialize(greater_or_equal(10, -5), context);
        REQUIRE(value == "(10 >= -5)");
    }
    {
        auto value = serialize(is_equal("ototo", "Hey"), context);
        REQUIRE(value == "('ototo' = 'Hey')");
    }
    {
        auto value = serialize(is_not_equal("lala", 7), context);
        REQUIRE(value == "('lala' != 7)");
    }
}
