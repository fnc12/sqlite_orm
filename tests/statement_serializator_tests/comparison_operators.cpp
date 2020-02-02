#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator comparison operators") {
    internal::serializator_context_base context;
    SECTION("lesser_than") {
        std::string value;
        SECTION("func") {
            value = serialize(lesser_than(4, 5), context);
        }
        SECTION("short func") {
            value = serialize(lt(4, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(4) < 5, context);
        }
        REQUIRE(value == "(4 < 5)");
    }
    SECTION("lesser_or_equal") {
        std::string value;
        SECTION("func") {
            value = serialize(lesser_or_equal(10, 15), context);
        }
        SECTION("short func") {
            value = serialize(le(10, 15), context);
        }
        SECTION("operator") {
            value = serialize(c(10) <= 15, context);
        }
        REQUIRE(value == "(10 <= 15)");
    }
    SECTION("greater_than") {
        std::string value;
        SECTION("func") {
            value = serialize(greater_than(1, 0.5), context);
        }
        SECTION("short func") {
            value = serialize(gt(1, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(1) > 0.5, context);
        }
        REQUIRE(value == "(1 > 0.5)");
    }
    SECTION("greater_or_equal") {
        std::string value;
        SECTION("func") {
            value = serialize(greater_or_equal(10, -5), context);
        }
        SECTION("short func") {
            value = serialize(ge(10, -5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) >= -5, context);
        }
        REQUIRE(value == "(10 >= -5)");
    }
    SECTION("is_equal") {
        std::string value;
        SECTION("func") {
            value = serialize(is_equal("ototo", "Hey"), context);
        }
        SECTION("short func") {
            value = serialize(eq("ototo", "Hey"), context);
        }
        SECTION("operator") {
            value = serialize(c("ototo") == "Hey", context);
        }
        REQUIRE(value == "('ototo' = 'Hey')");
    }
    SECTION("is_not_equal") {
        std::string value;
        SECTION("func") {
            value = serialize(is_not_equal("lala", 7), context);
        }
        SECTION("short func") {
            value = serialize(ne("lala", 7), context);
        }
        SECTION("operator") {
            value = serialize(c("lala") != 7, context);
        }
        REQUIRE(value == "('lala' != 7)");
    }
}
