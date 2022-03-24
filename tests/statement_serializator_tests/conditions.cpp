#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <string>

using namespace sqlite_orm;

TEST_CASE("statement_serializator conditions") {
    internal::storage_impl<> storage;
    internal::serializator_context<decltype(storage)> context{storage};

    std::string value, expected;

    SECTION("order_by") {
        SECTION("expression") {
            context.replace_bindable_with_question = true;
            value = serialize(order_by(c(1) == 0), context);
            expected = "ORDER BY (? = ?)";
        }
        SECTION("bindable") {
            context.replace_bindable_with_question = true;
            int n = 42;
            value = serialize(order_by(n), context);
            expected = "ORDER BY ?";
        }
        SECTION("numeric column alias") {
            context.replace_bindable_with_question = true;
            value = serialize(order_by(get<colalias_1>()), context);
            expected = "ORDER BY 1";
        }
        SECTION("sole column alias") {
            context.replace_bindable_with_question = true;
            value = serialize(order_by(get<colalias_a>()), context);
            expected = "ORDER BY a";
        }
        SECTION("column alias in expression") {
            context.replace_bindable_with_question = true;
            value = serialize(order_by(get<colalias_a>() > c(1)), context);
            expected = "ORDER BY (a > ?)";
        }
    }
    REQUIRE(value == expected);
}
