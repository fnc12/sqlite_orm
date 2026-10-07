#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("cross_join") {
    using internal::serialize;

    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    std::string expected;
    SECTION("straight") {
        auto node = cross_join<User>();
        value = serialize(node, context);
        expected = R"(CROSS JOIN "users")";
    }
    SECTION("alias") {
        using user_s = alias_s<User>;
        auto node = cross_join<user_s>();
        value = serialize(node, context);
        expected = R"(CROSS JOIN "users" "s")";
    }
    REQUIRE(value == expected);
}
