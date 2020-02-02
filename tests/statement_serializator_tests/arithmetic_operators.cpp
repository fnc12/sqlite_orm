#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator arithmetic operators") {
    internal::serializator_context_base context;
    SECTION("add") {
        std::string value;
        SECTION("func") {
            value = serialize(add(3, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(3) + 5, context);
        }
        REQUIRE(value == "(3 + 5)");
    }
    SECTION("sub") {
        std::string value;
        SECTION("func") {
            value = serialize(sub(5, -9), context);
        }
        SECTION("operator") {
            value = serialize(c(5) - -9, context);
        }
        REQUIRE(value == "(5 - -9)");
    }
    SECTION("mul") {
        std::string value;
        SECTION("func") {
            value = serialize(mul(10, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) * 0.5, context);
        }
        REQUIRE(value == "(10 * 0.5)");
    }
    SECTION("div") {
        std::string value;
        SECTION("func") {
            value = serialize(sqlite_orm::div(10, 2), context);
        }
        SECTION("operator") {
            value = serialize(c(10) / 2, context);
        }
        REQUIRE(value == "(10 / 2)");
    }
    SECTION("mod") {
        std::string value;
        SECTION("func") {
            value = serialize(mod(20, 3), context);
        }
        SECTION("operator") {
            value = serialize(c(20) % 3, context);
        }
        REQUIRE(value == "(20 % 3)");
    }
}
