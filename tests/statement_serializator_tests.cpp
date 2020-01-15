#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator") {
    using internal::serialize;
    {
        auto value = serialize(lesser_than(4, 5));
        REQUIRE(value == "(4 < 5)");
    }
    {
        auto value = serialize(lesser_or_equal(10, 15));
        REQUIRE(value == "(10 <= 15)");
    }
    {
        auto value = serialize(greater_than(1, 0.5));
        REQUIRE(value == "(1 > 0.5)");
    }
    {
        auto value = serialize(greater_or_equal(10, -5));
        REQUIRE(value == "(10 >= -5)");
    }
    {
        auto value = serialize(is_equal("ototo", "Hey"));
        REQUIRE(value == "('ototo' = 'Hey')");
    }
    {
        auto value = serialize(is_not_equal("lala", 7));
        REQUIRE(value == "('lala' != 7)");
    }
    {
        auto value = serialize(length("hi"));
        REQUIRE(value == "LENGTH('hi')");
    }
    {
        auto value = serialize(sqlite_orm::abs(-100));
        REQUIRE(value == "ABS(-100)");
    }
    {
        auto value = serialize(lower("dancefloor"));
        REQUIRE(value == "LOWER('dancefloor')");
    }
    {
        auto value = serialize(upper("call"));
        REQUIRE(value == "UPPER('call')");
    }
    {
        auto value = serialize(changes());
        REQUIRE(value == "CHANGES()");
    }
    {
        auto value = serialize(trim("hey"));
        REQUIRE(value == "TRIM('hey')");
    }
    {
        auto value = serialize(trim("hey", "h"));
        REQUIRE(value == "TRIM('hey', 'h')");
    }
    {
        auto value = serialize(ltrim("hey"));
        REQUIRE(value == "LTRIM('hey')");
    }
    {
        auto value = serialize(ltrim("hey", "h"));
        REQUIRE(value == "LTRIM('hey', 'h')");
    }
    {
        auto value = serialize(rtrim("hey"));
        REQUIRE(value == "RTRIM('hey')");
    }
    {
        auto value = serialize(rtrim("hey", "h"));
        REQUIRE(value == "RTRIM('hey', 'h')");
    }
    {
        auto value = serialize(hex("love"));
        REQUIRE(value == "HEX('love')");
    }
    {
        auto value = serialize(quote("one"));
        REQUIRE(value == "QUOTE('one')");
    }
    {
        auto value = serialize(randomblob(5));
        REQUIRE(value == "RANDOMBLOB(5)");
    }
    {
        auto value = serialize(instr("hi", "i"));
        REQUIRE(value == "INSTR('hi', 'i')");
    }
    {
        auto value = serialize(replace("contigo", "o", "a"));
        REQUIRE(value == "REPLACE('contigo', 'o', 'a')");
    }
    {
        auto value = serialize(sqlite_orm::round(10.5));
        REQUIRE(value == "ROUND(10.5)");
    }
    {
        auto value = serialize(sqlite_orm::round(10.5, 0.5));
        REQUIRE(value == "ROUND(10.5, 0.5)");
    }
}
