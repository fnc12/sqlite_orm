#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#if SQLITE_VERSION_NUMBER >= 3009000 || defined(SQLITE_ORM_ENABLE_FTS5)
using namespace sqlite_orm;

TEST_CASE("statement_serializer match") {
    struct User {
        int id = 0;
        std::string name;

        using hidden = fts5::hidden_fields_of<User>;
    };
    constexpr auto user_table = c<User>();

    auto table =
        make_virtual_table("users", using_fts5(make_column("id", &User::id), make_column("name", &User::name)));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    SECTION("match using explicit template parameter") {
        auto node = match<User>("Claude");
        auto value = serialize(node, context);
        REQUIRE(value == R"("users" MATCH 'Claude')");
    }
    SECTION("match any column") {
        auto node = match(user_table->*&fts5::hidden::any, "Claude");
        auto value = serialize(node, context);
        REQUIRE(value == R"("users" MATCH 'Claude')");
    }
    SECTION("match any column, rebound") {
        auto node = match(User::hidden::any_field, "Claude");
        auto value = serialize(node, context);
        REQUIRE(value == R"("users" MATCH 'Claude')");
    }
    SECTION("match specific column") {
        auto node = match(&User::name, "Claude");
        auto value = serialize(node, context);
        REQUIRE(value == R"("name" MATCH 'Claude')");
    }
}
#endif
