#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer remove") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializer_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    auto statement = remove<User>(5);
    SECTION("with question marks") {
        context.replace_bindable_with_question = true;
        expected = "DELETE FROM 'users' WHERE \"id\" = ?";
    }
    SECTION("without question marks") {
        context.replace_bindable_with_question = false;
        expected = "DELETE FROM 'users' WHERE \"id\" = 5";
    }
    value = serialize(statement, context);

    REQUIRE(value == expected);
}
