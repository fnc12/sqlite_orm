#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer arithmetic operators") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    decltype(value) expected;
    SECTION("add") {
        SECTION("func") {
            value = serialize(add(3, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(3) + 5, context);
        }
        expected = "(3 + 5)";
    }
    SECTION("sub") {
        SECTION("func") {
            value = serialize(sub(5, -9), context);
        }
        SECTION("operator") {
            value = serialize(c(5) - -9, context);
        }
        expected = "(5 - -9)";
    }
    SECTION("mul") {
        SECTION("func") {
            value = serialize(mul(10, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) * 0.5, context);
        }
        expected = "(10 * 0.5)";
    }
    SECTION("div") {
        SECTION("func") {
            value = serialize(sqlite_orm::div(10, 2), context);
        }
        SECTION("operator") {
            value = serialize(c(10) / 2, context);
        }
        expected = "(10 / 2)";
    }
    SECTION("mod") {
        SECTION("func") {
            value = serialize(mod(20, 3), context);
        }
        SECTION("operator") {
            value = serialize(c(20) % 3, context);
        }
        expected = "(20 % 3)";
    }
    REQUIRE(value == expected);
}
