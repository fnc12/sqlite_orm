#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator") {
    using internal::serialize;
    internal::serializator_context context;
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
    {
        auto value = serialize(length("hi"), context);
        REQUIRE(value == "LENGTH('hi')");
    }
    {
        auto value = serialize(sqlite_orm::abs(-100), context);
        REQUIRE(value == "ABS(-100)");
    }
    {
        auto value = serialize(lower("dancefloor"), context);
        REQUIRE(value == "LOWER('dancefloor')");
    }
    {
        auto value = serialize(upper("call"), context);
        REQUIRE(value == "UPPER('call')");
    }
    {
        auto value = serialize(changes(), context);
        REQUIRE(value == "CHANGES()");
    }
    {
        auto value = serialize(trim("hey"), context);
        REQUIRE(value == "TRIM('hey')");
    }
    {
        auto value = serialize(trim("hey", "h"), context);
        REQUIRE(value == "TRIM('hey', 'h')");
    }
    {
        auto value = serialize(ltrim("hey"), context);
        REQUIRE(value == "LTRIM('hey')");
    }
    {
        auto value = serialize(ltrim("hey", "h"), context);
        REQUIRE(value == "LTRIM('hey', 'h')");
    }
    {
        auto value = serialize(rtrim("hey"), context);
        REQUIRE(value == "RTRIM('hey')");
    }
    {
        auto value = serialize(rtrim("hey", "h"), context);
        REQUIRE(value == "RTRIM('hey', 'h')");
    }
    {
        auto value = serialize(hex("love"), context);
        REQUIRE(value == "HEX('love')");
    }
    {
        auto value = serialize(quote("one"), context);
        REQUIRE(value == "QUOTE('one')");
    }
    {
        auto value = serialize(randomblob(5), context);
        REQUIRE(value == "RANDOMBLOB(5)");
    }
    {
        auto value = serialize(instr("hi", "i"), context);
        REQUIRE(value == "INSTR('hi', 'i')");
    }
    {
        auto value = serialize(replace("contigo", "o", "a"), context);
        REQUIRE(value == "REPLACE('contigo', 'o', 'a')");
    }
    {
        auto value = serialize(sqlite_orm::round(10.5), context);
        REQUIRE(value == "ROUND(10.5)");
    }
    {
        auto value = serialize(sqlite_orm::round(10.5, 0.5), context);
        REQUIRE(value == "ROUND(10.5, 0.5)");
    }
#if SQLITE_VERSION_NUMBER >= 3007016
    {
        auto value = serialize(char_(40, 45), context);
        REQUIRE(value == "CHAR(40, 45)");
    }
    {
        auto value = serialize(sqlite_orm::random(), context);
        REQUIRE(value == "RANDOM()");
    }
#endif
    {
        auto value = serialize(coalesce<std::string>(10, 15), context);
        REQUIRE(value == "COALESCE(10, 15)");
    }
    {
        auto value = serialize(date("now"), context);
        REQUIRE(value == "DATE('now')");
    }
    {
        auto value = serialize(time("12:00", "localtime"), context);
        REQUIRE(value == "TIME('12:00', 'localtime')");
    }
    {
        auto value = serialize(datetime("now"), context);
        REQUIRE(value == "DATETIME('now')");
    }
    {
        auto value = serialize(julianday("now"), context);
        REQUIRE(value == "JULIANDAY('now')");
    }
    {
        auto value = serialize(zeroblob(5), context);
        REQUIRE(value == "ZEROBLOB(5)");
    }
    {
        auto value = serialize(substr("Zara", 2), context);
        REQUIRE(value == "SUBSTR('Zara', 2)");
    }
    {
        auto value = serialize(substr("Natasha", 3, 2), context);
        REQUIRE(value == "SUBSTR('Natasha', 3, 2)");
    }
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
