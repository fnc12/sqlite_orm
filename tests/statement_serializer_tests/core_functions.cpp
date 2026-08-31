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
#if SQLITE_VERSION_NUMBER >= 3034000
    SECTION("SUBSTRING(X,Y)") {
        constexpr auto expression = substring("Zara", 2);
        expected = "SUBSTRING('Zara', 2)";
        value = serialize(expression, context);
    }
    SECTION("SUBSTRING(X,Y,Z)") {
        constexpr auto expression = substring("Natasha", 3, 2);
        expected = "SUBSTRING('Natasha', 3, 2)";
        value = serialize(expression, context);
    }
#endif
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

TEST_CASE("statement_serializer newer core functions") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    context.use_parentheses = false;
    std::string value;
    decltype(value) expected;
#if SQLITE_VERSION_NUMBER >= 3008003
    SECTION("PRINTF") {
        auto expression = sqlite_orm::printf("%d", 1);
        expected = "PRINTF('%d', 1)";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3032000
    SECTION("IIF") {
        auto expression = iif(c(2) > 1, "greater", "less");
        expected = "IIF(2 > 1, 'greater', 'less')";
        value = serialize(expression, context);
    }
#endif
    SECTION("SQLITE_VERSION") {
        constexpr auto expression = sqlite_version();
        expected = "SQLITE_VERSION()";
        value = serialize(expression, context);
    }
    SECTION("SQLITE_SOURCE_ID") {
        constexpr auto expression = sqlite_source_id();
        expected = "SQLITE_SOURCE_ID()";
        value = serialize(expression, context);
    }
#ifndef SQLITE_OMIT_COMPILEOPTION_DIAGS
    SECTION("SQLITE_COMPILEOPTION_USED") {
        constexpr auto expression = sqlite_compileoption_used("THREADSAFE");
        expected = "SQLITE_COMPILEOPTION_USED('THREADSAFE')";
        value = serialize(expression, context);
    }
    SECTION("SQLITE_COMPILEOPTION_GET") {
        constexpr auto expression = sqlite_compileoption_get(1);
        expected = "SQLITE_COMPILEOPTION_GET(1)";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008001
    SECTION("LIKELIHOOD") {
        //  constexpr: an out-of-range probability would fail right here, at compile time
        constexpr auto expression = likelihood(20, 0.0625);
        expected = "LIKELIHOOD(20, 0.0625)";
        value = serialize(expression, context);
    }
    SECTION("UNLIKELY") {
        auto expression = unlikely(20);
        expected = "UNLIKELY(20)";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3008006
    SECTION("LIKELY") {
        auto expression = likely(20);
        expected = "LIKELY(20)";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3035000
    SECTION("SIGN") {
        auto expression = sign(-3);
        expected = "SIGN(-3)";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3038000
    SECTION("FORMAT") {
        auto expression = format("%d", 1);
        expected = "FORMAT('%d', 1)";
        value = serialize(expression, context);
    }
    SECTION("UNIXEPOCH") {
        auto expression = unixepoch("now");
        expected = "UNIXEPOCH('now')";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3041000
    SECTION("UNHEX") {
        auto expression = unhex("3637");
        expected = "UNHEX('3637')";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3043000
    SECTION("OCTET_LENGTH") {
        auto expression = octet_length("hi");
        expected = "OCTET_LENGTH('hi')";
        value = serialize(expression, context);
    }
    SECTION("TIMEDIFF") {
        auto expression = timediff("2026-08-08", "2026-08-07");
        expected = "TIMEDIFF('2026-08-08', '2026-08-07')";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3044000
    SECTION("CONCAT") {
        auto expression = concat("one", 2);
        expected = "CONCAT('one', 2)";
        value = serialize(expression, context);
    }
    SECTION("CONCAT_WS") {
        auto expression = concat_ws("-", "one", "two");
        expected = "CONCAT_WS('-', 'one', 'two')";
        value = serialize(expression, context);
    }
    SECTION("STRING_AGG") {
        auto expression = string_agg("x", ",");
        expected = "STRING_AGG('x', ',')";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3048000
    SECTION("IF") {
        auto expression = if_(c(2) > 1, "greater", "less");
        expected = "IF(2 > 1, 'greater', 'less')";
        value = serialize(expression, context);
    }
#endif
#if SQLITE_VERSION_NUMBER >= 3050000
    SECTION("UNISTR") {
        auto expression = unistr("a\\u0062c");
        expected = "UNISTR('a\\u0062c')";
        value = serialize(expression, context);
    }
    SECTION("UNISTR_QUOTE") {
        auto expression = unistr_quote("abc");
        expected = "UNISTR_QUOTE('abc')";
        value = serialize(expression, context);
    }
#endif
#ifdef SQLITE_ENABLE_PERCENTILE
    SECTION("MEDIAN") {
        auto expression = median(1);
        expected = "MEDIAN(1)";
        value = serialize(expression, context);
    }
    SECTION("PERCENTILE") {
        auto expression = percentile(1, 50);
        expected = "PERCENTILE(1, 50)";
        value = serialize(expression, context);
    }
    SECTION("PERCENTILE_CONT") {
        auto expression = percentile_cont(1, 0.5);
        expected = "PERCENTILE_CONT(1, 0.5)";
        value = serialize(expression, context);
    }
    SECTION("PERCENTILE_DISC") {
        auto expression = percentile_disc(1, 0.5);
        expected = "PERCENTILE_DISC(1, 0.5)";
        value = serialize(expression, context);
    }
#endif
    REQUIRE(value == expected);
}
