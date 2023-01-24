#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer rowid") {
    {
        internal::db_objects_tuple<> storage;
        internal::serializer_context<internal::db_objects_tuple<>> context{storage};
        auto value = serialize(rowid(), context);
        REQUIRE(value == "rowid");
    }
}
