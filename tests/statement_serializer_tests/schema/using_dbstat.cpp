#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("statement_serializer dbstat") {
    struct mystat : sqlite_orm::dbstat {};
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    constexpr orm_table_reference auto mystat_table = c<mystat>();
#endif

    std::string value;
    std::string expected;
    using db_objects_t = internal::db_objects_tuple<>;
    const db_objects_t dbObjects{};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    SECTION("default") {
        auto expression = make_virtual_table<mystat>("mystat", using_dbstat());
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat")";
    }
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("table reference") {
        auto expression = make_virtual_table<mystat_table>("mystat", using_dbstat());
        value = serialize(expression, context);
        expected = R"(CREATE VIRTUAL TABLE IF NOT EXISTS "mystat" USING "dbstat")";
    }
#endif
    REQUIRE(value == expected);
}
#endif
