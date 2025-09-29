#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer rtree") {
    struct DemoIndex {
        int64 id;
        double minX, maxX;
        double minY, maxY;
        double minZ, maxZ;
        double minA, maxA;
        double minB, maxB;
    };
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto demo_index = c<DemoIndex>();
#endif

    std::string value;
    std::string expected;
    using db_objects_t = internal::db_objects_tuple<>;
    const db_objects_t dbObjects{};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    SECTION("1 dimension") {
        auto expression = make_virtual_table("demo_index",
                                             using_rtree(make_column("id", &DemoIndex::id),
                                                         make_column("minX", &DemoIndex::minX),
                                                         make_column("maxX", &DemoIndex::maxX)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id", "minX", "maxX"))";
    }
    SECTION("explicit object") {
        auto expression = make_virtual_table("demo_index",
                                             using_rtree<DemoIndex>(make_column("id", &DemoIndex::id),
                                                                    make_column("minX", &DemoIndex::minX),
                                                                    make_column("maxX", &DemoIndex::maxX)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id", "minX", "maxX"))";
    }
    SECTION("5 dimensions") {
        auto expression = make_virtual_table("demo_index",
                                             using_rtree(make_column("id", &DemoIndex::id),
                                                         make_column("minX", &DemoIndex::minX),
                                                         make_column("maxX", &DemoIndex::maxX),
                                                         make_column("minY", &DemoIndex::minY),
                                                         make_column("maxY", &DemoIndex::maxY),
                                                         make_column("minZ", &DemoIndex::minZ),
                                                         make_column("maxZ", &DemoIndex::maxZ),
                                                         make_column("minA", &DemoIndex::minA),
                                                         make_column("maxA", &DemoIndex::maxA),
                                                         make_column("minB", &DemoIndex::minB),
                                                         make_column("maxB", &DemoIndex::maxB)));
        value = serialize(expression, context);
        expected =
            R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id", "minX", "maxX", "minY", "maxY", "minZ", "maxZ", "minA", "maxA", "minB", "maxB"))";
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference") {
        auto expression = make_virtual_table("demo_index",
                                             using_rtree<demo_index>(make_column("id", &DemoIndex::id),
                                                                     make_column("minX", &DemoIndex::minX),
                                                                     make_column("maxX", &DemoIndex::maxX)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id", "minX", "maxX"))";
    }
#endif
    REQUIRE(value == expected);
}
