#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer table_t") {
    struct User {
        int id = 0;
        std::string name;
    };
    std::string value;
    std::string expected;
    SECTION("simple") {
        auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
        using db_objects_t = internal::db_objects_tuple<decltype(table)>;
        auto dbObjects = db_objects_t{table};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
        value = internal::serialize(table, context);
        expected = "CREATE TABLE \"users\" (\"id\" INTEGER NOT NULL, \"name\" TEXT NOT NULL)";
    }
    SECTION("without_rowid") {
        auto table =
            make_table("users", make_column("id", &User::id), make_column("name", &User::name)).without_rowid();
        using db_objects_t = internal::db_objects_tuple<decltype(table)>;
        auto dbObjects = db_objects_t{table};
        using context_t = internal::serializer_context<db_objects_t>;
        context_t context{dbObjects};
        value = internal::serialize(table, context);
        expected = "CREATE TABLE \"users\" (\"id\" INTEGER NOT NULL, \"name\" TEXT NOT NULL) WITHOUT ROWID";
    }
    REQUIRE(value == expected);
}
