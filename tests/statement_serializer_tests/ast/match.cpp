#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000
using namespace sqlite_orm;

TEST_CASE("statement_serializer match") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table =
        make_virtual_table("users", using_fts5(make_column("id", &User::id), make_column("name", &User::name)));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    auto node = match<User>("Claude");
    auto value = serialize(node, context);
    REQUIRE(value == R"("users" MATCH 'Claude')");
}
#endif
