#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator raise") {
    using internal::serialize;

    std::string value;
    decltype(value) expected;

    internal::storage_impl<> storage;
    internal::serializator_context<internal::storage_impl<>> context{storage};
    SECTION("ignore") {
        auto expression = raise_ignore();
        value = serialize(expression, context);
        expected = "RAISE(IGNORE)";
    }
    SECTION("rollback") {
        auto expression = raise_rollback("no rap");
        value = serialize(expression, context);
        expected = "RAISE(ROLLBACK, 'no rap')";
    }
    SECTION("abort") {
        auto expression = raise_abort("no rap");
        value = serialize(expression, context);
        expected = "RAISE(ABORT, 'no rap')";
    }
    SECTION("fail") {
        auto expression = raise_fail("no rap");
        value = serialize(expression, context);
        expected = "RAISE(FAIL, 'no rap')";
    }
    REQUIRE(value == expected);
}
