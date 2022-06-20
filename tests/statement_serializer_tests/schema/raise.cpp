#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer raise") {
    using internal::serialize;

    std::string value;
    decltype(value) expected;

    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
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
