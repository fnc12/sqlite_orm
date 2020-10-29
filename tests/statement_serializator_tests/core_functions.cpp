#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator core functions") {
    internal::serializator_context_base context;
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
        auto value = serialize(total_changes(), context);
        REQUIRE(value == "TOTAL_CHANGES()");
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
        auto value = serialize(strftime("%s", "2014-10-07 02:34:56"), context);
        REQUIRE(value == "STRFTIME('%s', '2014-10-07 02:34:56')");
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
#ifdef SQLITE_SOUNDEX
        auto value = serialize(soundex("Vaso"), context);
        REQUIRE(value == "SOUNDEX('Vaso')");
#endif
    }
}
