#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator autoincrement") {
    internal::serializator_context_base context;
    auto autoinc = autoincrement();
    auto value = serialize(autoinc, context);
    REQUIRE(value == "AUTOINCREMENT");
}
