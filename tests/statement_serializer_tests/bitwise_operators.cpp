#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer bitwise operators") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    decltype(value) expected;
    SECTION("bitwise_or") {
        SECTION("func") {
            value = serialize(bitwise_or(3, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(3) | 5, context);
        }
        expected = "3 | 5";
    }
    SECTION("bitwise_and") {
        SECTION("func") {
            value = serialize(bitwise_and(5, -9), context);
        }
        SECTION("operator") {
            value = serialize(c(5) & -9, context);
        }
        expected = "5 & -9";
    }
    SECTION("bitwise_shift_left") {
        SECTION("func") {
            value = serialize(bitwise_shift_left(10, 1), context);
        }
        SECTION("operator") {
            value = serialize(c(10) << 1, context);
        }
        expected = "10 << 1";
    }
    SECTION("bitwise_shift_right") {
        SECTION("func") {
            value = serialize(bitwise_shift_right(10, 2), context);
        }
        SECTION("operator") {
            value = serialize(c(10) >> 2, context);
        }
        expected = "10 >> 2";
    }
    SECTION("bitwise_not") {
        SECTION("func") {
            value = serialize(bitwise_not(20), context);
        }
        SECTION("operator") {
            value = serialize(~c(20), context);
        }
        expected = "~20";
    }
    SECTION("parentheses keeping order of precedence") {
        SECTION("1") {
            value = serialize(c(4) << 5 << 3, context);
            expected = "(4 << 5) << 3";
        }
        SECTION("2") {
            value = serialize(4 << (c(5) << 3), context);
            expected = "4 << (5 << 3)";
        }
        SECTION("3") {
            value = serialize(4 | ~c(5) & 3 | 1, context);
            expected = "(4 | (~5 & 3)) | 1";
        }
    }
    REQUIRE(value == expected);
}
