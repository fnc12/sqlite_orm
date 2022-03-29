#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer new/old") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializer_context<storage_impl_t>;
    context_t context{storageImpl};
    std::string value;
    decltype(value) expected;
    SECTION("new") {
        auto expression = new_(&User::id);
        value = serialize(expression, context);
        expected = "NEW.\"id\"";
    }
    SECTION("old") {
        auto expression = old(&User::id);
        value = serialize(expression, context);
        expected = "OLD.\"id\"";
    }
    REQUIRE(value == expected);
}
