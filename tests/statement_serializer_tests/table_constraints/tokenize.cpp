#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer tokenize") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("porter ascii") {
        auto node = tokenize("porter ascii");
        value = serialize(node, context);
        expected = "tokenize = 'porter ascii'";
    }
    SECTION("porter ascii") {
        auto node = tokenize("unicode61 remove_diacritics 1");
        value = serialize(node, context);
        expected = "tokenize = 'unicode61 remove_diacritics 1'";
    }
    REQUIRE(value == expected);
}
