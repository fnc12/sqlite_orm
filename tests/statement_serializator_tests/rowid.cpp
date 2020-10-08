#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator rowid") {
    {
        internal::serializator_context_base context;
        auto value = serialize(rowid(), context);
        REQUIRE(value == "rowid");
    }
}
