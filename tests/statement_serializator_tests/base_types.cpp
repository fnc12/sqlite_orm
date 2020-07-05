#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator base types") {
    internal::serializator_context_base context;
    SECTION("std::string") {
        std::string str("calma");
        SECTION("no question") {
            auto value = serialize(str, context);
            REQUIRE(value == "\"calma\"");
        }
        SECTION("question") {
            context.replace_bindable_with_question = true;
            auto value = serialize(str, context);
            REQUIRE(value == "?");
        }
    }
    SECTION("const char *") {
        const char *str = "baby";
        SECTION("no question") {
            auto value = serialize(str, context);
            REQUIRE(value == "\'baby\'");
        }
        SECTION("question") {
            context.replace_bindable_with_question = true;
            auto value = serialize(str, context);
            REQUIRE(value == "?");
        }
    }
}
