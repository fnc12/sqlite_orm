#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
#ifdef SQLITE_ORM_REFLECTION_SUPPORTED
using namespace sqlite_orm;
using internal::serialize;

namespace {
    struct[[= "user_view"_orm_name]] UserViewSerializerTests {
        int id = 0;
        std::string name;
    };
}

TEST_CASE("view statement_serializer") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto table = make_table<User>("user", make_column("id", &User::id), make_column("name", &User::name));
    auto view = make_view<UserViewSerializerTests>(select(asterisk<User>(true)));
    using db_objects_t = internal::db_objects_tuple<decltype(table), decltype(view)>;
    const db_objects_t dbObjects{table, view};
    using context_t = internal::serializer_context<db_objects_t>;
    const context_t context{dbObjects};

    SECTION("create") {
        std::string value = serialize(view, context);
        REQUIRE(value == R"(CREATE VIEW "user_view" ("id", "name") AS SELECT "user"."id", "user"."name" FROM "user")");
    }
    SECTION("as object") {
        auto expression = select(object<UserViewSerializerTests>());
        expression.highest_level = true;
        std::string value = serialize(expression, context);
        REQUIRE(value == R"(SELECT "user_view".* FROM "user_view")");
    }
    SECTION("asterisk") {
        auto expression = select(asterisk<UserViewSerializerTests>());
        expression.highest_level = true;
        std::string value = serialize(expression, context);
        REQUIRE(value == R"(SELECT "user_view".* FROM "user_view")");
    }
}
#endif
#endif
