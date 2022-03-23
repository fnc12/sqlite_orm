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
        SECTION("literal") {
            context.replace_bindable_with_question = true;
            value = serialize(order_by(1), context);
            expected = "ORDER BY 1";
        }
    }
    REQUIRE(value == expected);
}
