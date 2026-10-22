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
        //  the RANK() window function, which is applied by an OVER clause
        auto node = rank();
        value = serialize(node, context);
        expected = "RANK()";
    }
#ifdef SQLITE_ORM_FTS5_SUPPORTED
    SECTION("order by rank") {
        //  the deprecated spelling of the hidden FTS5 rank column
        auto node = order_by(rank());
        value = serialize(node, context);
        expected = "ORDER BY rank";
    }
#endif
    REQUIRE(value == expected);
}
