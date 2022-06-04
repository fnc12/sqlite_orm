#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer unique") {
    internal::schema_objects<> storage;
    internal::serializer_context<internal::schema_objects<>> context{storage};
    auto un = unique();
    auto value = serialize(un, context);
    REQUIRE(value == "UNIQUE");
}
