#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator base types") {
    internal::serializator_context_base context;
    std::string stringValue;
    decltype(stringValue) expected;
    SECTION("std::string") {
        std::string str("calma");
        SECTION("no question") {
            stringValue = serialize(str, context);
            expected = "\'calma\'";
        }
        SECTION("question") {
            context.replace_bindable_with_question = true;
            stringValue = serialize(str, context);
            expected = "?";
        }
    }
    SECTION("const char *") {
        const char* str = "baby";
        SECTION("no question") {
            stringValue = serialize(str, context);
            expected = "\'baby\'";
        }
        SECTION("question") {
            context.replace_bindable_with_question = true;
            stringValue = serialize(str, context);
            expected = "?";
        }
    }
    REQUIRE(stringValue == expected);
}
