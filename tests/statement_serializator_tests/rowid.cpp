#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator rowid") {
    {
        internal::storage_impl<> storage;
        internal::serializator_context<internal::storage_impl<>> context{storage};
        auto value = serialize(rowid(), context);
        REQUIRE(value == "rowid");
    }
}
