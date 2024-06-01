#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer content") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("empty") {
        auto node = content("");
        value = serialize(node, context);
        expected = "content=''";
    }
    REQUIRE(value == expected);
}

TEST_CASE("statement_serializer table_content") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    auto node = content<User>();
    auto value = internal::serialize(node, context);
    REQUIRE(value == R"(content="users")");
}
