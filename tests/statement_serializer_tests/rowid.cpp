#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer rowid") {
    {
        internal::storage_impl<> storage;
        internal::serializer_context<internal::storage_impl<>> context{storage};
        auto value = serialize(rowid(), context);
        REQUIRE(value == "rowid");
    }
}
