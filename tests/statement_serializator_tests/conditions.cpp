#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator order by") {
    internal::serializator_context_base context;

    SECTION("dump") {
        context.replace_bindable_with_question = false;
        auto ast = order_by(2);
        auto value = serialize(ast, context);
        REQUIRE(value == "ORDER BY 2");
    }
    SECTION("bindable") {
        context.replace_bindable_with_question = true;
        auto ast = order_by(2);
        auto value = serialize(ast, context);
        REQUIRE(value == "ORDER BY ?");
    }
}
