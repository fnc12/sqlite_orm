#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator logical operators") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    auto storageImpl = storage_impl_t{table};
    using context_t = internal::serializator_context<storage_impl_t>;
    context_t context{storageImpl};

    SECTION("static in") {
        auto inValue = c(&User::id).in(1, 2, 3);
        auto stringValue = internal::serialize(inValue, context);
        REQUIRE(stringValue == "\"id\" IN (1, 2, 3)");
    }
    SECTION("static not in") {
        auto inValue = c(&User::id).not_in(1, 2, 3);
        auto stringValue = internal::serialize(inValue, context);
        REQUIRE(stringValue == "\"id\" NOT IN (1, 2, 3)");
    }
    SECTION("dynamic in") {
        auto inValue = in(&User::id, {1, 2, 3});
        auto stringValue = internal::serialize(inValue, context);
        REQUIRE(stringValue == "\"id\" IN (1, 2, 3)");
    }
    SECTION("dynamic not in") {
        auto inValue = not_in(&User::id, {1, 2, 3});
        auto stringValue = internal::serialize(inValue, context);
        REQUIRE(stringValue == "\"id\" NOT IN (1, 2, 3)");
    }
}
