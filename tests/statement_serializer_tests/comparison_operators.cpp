#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer comparison operators") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("lesser_than") {
        SECTION("func") {
            value = serialize(lesser_than(4, 5), context);
        }
        SECTION("short func") {
            value = serialize(lt(4, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(4) < 5, context);
        }
        expected = "(4 < 5)";
    }
    SECTION("lesser_or_equal") {
        SECTION("func") {
            value = serialize(lesser_or_equal(10, 15), context);
        }
        SECTION("short func") {
            value = serialize(le(10, 15), context);
        }
        SECTION("operator") {
            value = serialize(c(10) <= 15, context);
        }
        expected = "(10 <= 15)";
    }
    SECTION("greater_than") {
        SECTION("func") {
            value = serialize(greater_than(1, 0.5), context);
        }
        SECTION("short func") {
            value = serialize(gt(1, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(1) > 0.5, context);
        }
        expected = "(1 > 0.5)";
    }
    SECTION("greater_or_equal") {
        SECTION("func") {
            value = serialize(greater_or_equal(10, -5), context);
        }
        SECTION("short func") {
            value = serialize(ge(10, -5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) >= -5, context);
        }
        expected = "(10 >= -5)";
    }
    SECTION("is_equal") {
        SECTION("func") {
            value = serialize(is_equal("ototo", "Hey"), context);
        }
        SECTION("short func") {
            value = serialize(eq("ototo", "Hey"), context);
        }
        SECTION("operator") {
            value = serialize(c("ototo") == "Hey", context);
        }
        expected = "('ototo' = 'Hey')";
    }
    SECTION("is_not_equal") {
        SECTION("func") {
            value = serialize(is_not_equal("lala", 7), context);
        }
        SECTION("short func") {
            value = serialize(ne("lala", 7), context);
        }
        SECTION("operator") {
            value = serialize(c("lala") != 7, context);
        }
        expected = "('lala' != 7)";
    }
    REQUIRE(value == expected);
}
