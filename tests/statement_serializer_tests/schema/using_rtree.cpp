#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_RTREE
TEST_CASE("statement_serializer rtree") {
    struct DemoIndex {
        int64 id;
        float minX, maxX;
        float minY, maxY;

        std::string objname;
        std::string objtype;
        std::vector<char> boundary;
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
                                             using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                                         make_column("minX", &DemoIndex::minX),
                                                         make_column("maxX", &DemoIndex::maxX)));
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id" PRIMARY KEY, "minX", "maxX"))";
    }
#if SQLITE_VERSION_NUMBER >= 3024000
    SECTION("auxiliary columns") {
        auto expression = make_virtual_table("demo_index",
                                             using_rtree(make_column("id", &DemoIndex::id, primary_key()),
                                                         make_column("minX", &DemoIndex::minX),
                                                         make_column("maxX", &DemoIndex::maxX),
                                                         make_column("objname", &DemoIndex::objname, auxiliary()),
                                                         make_column("objtype", &DemoIndex::objtype, auxiliary()),
                                                         make_column("boundary", &DemoIndex::boundary, auxiliary())));
        value = serialize(expression, context);
        expected =
            R"(CREATE VIRTUAL TABLE IF NOT EXISTS "demo_index" USING "rtree"("id" PRIMARY KEY, "minX", "maxX", +"objname", +"objtype", +"boundary"))";
    }
#endif
    REQUIRE(value == expected);
}
#endif
