#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer rank") {
    using db_objects_t = internal::db_objects_tuple<>;
    auto dbObjects = db_objects_t{};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    std::string expected;
    SECTION("rank") {
        auto node = rank();
        value = serialize(node, context);
        expected = "rank";
    }
    SECTION("order by rank") {
        auto node = order_by(rank());
        value = serialize(node, context);
        expected = "ORDER BY rank";
    }
    REQUIRE(value == expected);
}
