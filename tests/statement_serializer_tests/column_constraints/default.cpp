#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer default") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    decltype(value) expected;
    SECTION("int literal") {
        auto def = default_value(1);
        value = serialize(def, context);
        expected = "DEFAULT (1)";
    }
    SECTION("string literal") {
        auto def = default_value("hi");
        value = serialize(def, context);
        expected = "DEFAULT ('hi')";
    }
    SECTION("func") {
        auto def = default_value(datetime("now"));
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = "DEFAULT ((DATETIME('now')))";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = "DEFAULT (DATETIME('now'))";
        }
        value = serialize(def, context);
    }
    REQUIRE(value == expected);
}
