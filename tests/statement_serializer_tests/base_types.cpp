#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer base types") {
    internal::storage_impl<> storage;
    internal::serializer_context<internal::storage_impl<>> context{storage};
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
#ifdef SQLITE_ORM_STRING_VIEW_SUPPORTED
    SECTION("std::string_view") {
        std::string_view str = "agora";
        SECTION("no question") {
            stringValue = serialize(str, context);
            expected = "\'agora\'";
        }
        SECTION("question") {
            context.replace_bindable_with_question = true;
            stringValue = serialize(str, context);
            expected = "?";
        }
    }
#endif
    REQUIRE(stringValue == expected);
}
