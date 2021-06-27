#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator from_t") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    std::string value;
    decltype(value) expected;
    SECTION("without alias") {
        auto expression = from<User>();
        value = internal::serialize(expression, context);
        expected = "FROM 'users'";
    }
    SECTION("with alias") {
        auto expression = from<alias_u<User>>();
        value = internal::serialize(expression, context);
        expected = "FROM 'users' 'u'";
    }
    REQUIRE(value == expected);
}
