#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer remove_all") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    storage_impl_t storageImpl{table};
    using context_t = internal::serializer_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    std::string expected;

    SECTION("all") {
        auto statement = remove_all<User>();
        value = serialize(statement, context);
        expected = R"(DELETE FROM "users")";
    }
    SECTION("where") {
        auto statement = remove_all<User>(where(&User::id == c(1)));
        value = serialize(statement, context);
        expected = R"(DELETE FROM "users" WHERE (("id" = 1)))";
    }
    SECTION("conditions") {
        auto statement = remove_all<User>(where(&User::id == c(1)), limit(1));
        value = serialize(statement, context);
        expected = R"(DELETE FROM "users" WHERE (("id" = 1)) LIMIT 1)";
    }
    REQUIRE(value == expected);
}
