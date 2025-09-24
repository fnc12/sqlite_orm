#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("statement_serializer unindexed") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    auto node = unindexed();
    auto value = serialize(node, context);
    REQUIRE(value == "UNINDEXED");
}
#endif
