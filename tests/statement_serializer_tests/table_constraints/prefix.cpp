#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_FTS5_SUPPORTED
using namespace sqlite_orm;

TEST_CASE("statement_serializer prefix") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("2") {
        constexpr auto node = prefix(2);
        value = serialize(node, context);
        expected = "prefix=2";
    }
    SECTION("3") {
        constexpr auto node = prefix(3);
        value = serialize(node, context);
        expected = "prefix=3";
    }
    SECTION("2 3") {
        constexpr auto node = prefix("2 3");
        value = serialize(node, context);
        expected = "prefix='2 3'";
    }
    REQUIRE(value == expected);
}
#endif
