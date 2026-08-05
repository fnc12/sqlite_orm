#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer core functions") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    decltype(value) expected;
    SECTION("MAX(X,Y)") {
        constexpr auto expression = max(3, 4);
        context.use_parentheses = false;
        expected = "MAX(3, 4)";
        value = serialize(expression, context);
    }
    SECTION("MIN(X,Y)") {
        constexpr auto expression = min(3, 4);
        context.use_parentheses = false;
        expected = "MIN(3, 4)";
        value = serialize(expression, context);
    }
    SECTION("LENGTH") {
        constexpr auto expression = length("hi");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "LENGTH('hi')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "LENGTH('hi')";
        }
        value = serialize(expression, context);
    }
    SECTION("ABS") {
        constexpr auto expression = sqlite_orm::abs(-100);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "ABS(-100)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "ABS(-100)";
        }
        value = serialize(expression, context);
    }
    SECTION("LOWER") {
        constexpr auto expression = lower("dancefloor");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "LOWER('dancefloor')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "LOWER('dancefloor')";
        }
        value = serialize(expression, context);
    }
    SECTION("UPPER") {
        constexpr auto expression = upper("call");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "UPPER('call')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "UPPER('call')";
        }
        value = serialize(expression, context);
    }
    SECTION("TOTAL_CHANGES") {
        constexpr auto expression = total_changes();
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "TOTAL_CHANGES()";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "TOTAL_CHANGES()";
        }
        value = serialize(expression, context);
    }
    SECTION("CHANGES") {
        constexpr auto expression = changes();
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "CHANGES()";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "CHANGES()";
        }
        value = serialize(expression, context);
    }
    SECTION("TRIM(X)") {
        constexpr auto expression = trim("hey");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "TRIM('hey')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "TRIM('hey')";
        }
        value = serialize(expression, context);
    }
    SECTION("TRIM(X,Y)") {
        constexpr auto expression = trim("hey", "h");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "TRIM('hey', 'h')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "TRIM('hey', 'h')";
        }
        value = serialize(expression, context);
    }
    SECTION("LTRIM(X)") {
        constexpr auto expression = ltrim("hey");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "LTRIM('hey')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "LTRIM('hey')";
        }
        value = serialize(expression, context);
    }
    SECTION("LTRIM(X,Y)") {
        constexpr auto expression = ltrim("hey", "h");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "LTRIM('hey', 'h')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "LTRIM('hey', 'h')";
        }
        value = serialize(expression, context);
    }
    SECTION("RTRIM(X)") {
        constexpr auto expression = rtrim("hey");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "RTRIM('hey')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "RTRIM('hey')";
        }
        value = serialize(expression, context);
    }
    SECTION("RTRIM(X,Y)") {
        constexpr auto expression = rtrim("hey", "h");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "RTRIM('hey', 'h')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "RTRIM('hey', 'h')";
        }
        value = serialize(expression, context);
    }
    SECTION("HEX") {
        constexpr auto expression = hex("love");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "HEX('love')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "HEX('love')";
        }
        value = serialize(expression, context);
    }
    SECTION("QUOTE") {
        constexpr auto expression = quote("one");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "QUOTE('one')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "QUOTE('one')";
        }
        value = serialize(expression, context);
    }
    SECTION("RANDOMBLOB") {
        constexpr auto expression = randomblob(5);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "RANDOMBLOB(5)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "RANDOMBLOB(5)";
        }
        value = serialize(expression, context);
    }
    SECTION("INSTR") {
        constexpr auto expression = instr("hi", "i");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "INSTR('hi', 'i')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "INSTR('hi', 'i')";
        }
        value = serialize(expression, context);
    }
    SECTION("REPLACE") {
        constexpr auto expression = replace("contigo", "o", "a");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "REPLACE('contigo', 'o', 'a')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "REPLACE('contigo', 'o', 'a')";
        }
        value = serialize(expression, context);
    }
    SECTION("ROUND(X)") {
        constexpr auto expression = sqlite_orm::round(10.5);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "ROUND(10.5)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "ROUND(10.5)";
        }
        value = serialize(expression, context);
    }
    SECTION("ROUND(X,Y)") {
        constexpr auto expression = sqlite_orm::round(10.5, 0.5);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "ROUND(10.5, 0.5)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "ROUND(10.5, 0.5)";
        }
        value = serialize(expression, context);
    }
#if SQLITE_VERSION_NUMBER >= 3007016
    SECTION("CHAR") {
        constexpr auto expression = char_(40, 45);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "CHAR(40, 45)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "CHAR(40, 45)";
        }
        value = serialize(expression, context);
    }
    SECTION("RANDOM") {
        constexpr auto expression = sqlite_orm::random();
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "RANDOM()";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "RANDOM()";
        }
        value = serialize(expression, context);
    }
#endif
    SECTION("COALESCE") {
        constexpr auto expression = coalesce<std::string>(10, 15);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "COALESCE(10, 15)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "COALESCE(10, 15)";
        }
        value = serialize(expression, context);
    }
    SECTION("DATE") {
        constexpr auto expression = date("now");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "DATE('now')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "DATE('now')";
        }
        value = serialize(expression, context);
    }
    SECTION("TIME") {
        constexpr auto expression = time("12:00", "localtime");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "TIME('12:00', 'localtime')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "TIME('12:00', 'localtime')";
        }
        value = serialize(expression, context);
    }
    SECTION("DATETIME") {
        constexpr auto expression = datetime("now");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "DATETIME('now')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "DATETIME('now')";
        }
        value = serialize(expression, context);
    }
    SECTION("JULIANDAY") {
        constexpr auto expression = julianday("now");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "JULIANDAY('now')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "JULIANDAY('now')";
        }
        value = serialize(expression, context);
    }
    SECTION("STRFTIME") {
        constexpr auto expression = strftime("%s", "2014-10-07 02:34:56");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "STRFTIME('%s', '2014-10-07 02:34:56')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "STRFTIME('%s', '2014-10-07 02:34:56')";
        }
        value = serialize(expression, context);
    }
    SECTION("ZEROBLOB") {
        constexpr auto expression = zeroblob(5);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "ZEROBLOB(5)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "ZEROBLOB(5)";
        }
        value = serialize(expression, context);
    }
    SECTION("SUBSTR(X,Y)") {
        constexpr auto expression = substr("Zara", 2);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "SUBSTR('Zara', 2)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "SUBSTR('Zara', 2)";
        }
        value = serialize(expression, context);
    }
    SECTION("SUBSTR(X,Y,Z)") {
        constexpr auto expression = substr("Natasha", 3, 2);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "SUBSTR('Natasha', 3, 2)";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "SUBSTR('Natasha', 3, 2)";
        }
        value = serialize(expression, context);
    }
    SECTION("SOUNDEX") {
#ifdef SQLITE_SOUNDEX
        constexpr auto expression = soundex("Vaso");
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "SOUNDEX('Vaso')";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "SOUNDEX('Vaso')";
        }
        value = serialize(expression, context);
#endif
    }
    REQUIRE(value == expected);
}
