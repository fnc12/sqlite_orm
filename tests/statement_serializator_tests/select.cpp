#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator select_t") {
    using storage_impl_t = internal::storage_impl<>;
    storage_impl_t storageImpl;
    internal::serializator_context<storage_impl_t> context{storageImpl};
    SECTION("simple") {
        auto statement = select(1);
        SECTION("!highest_level") {
            statement.highest_level = false;
            auto stringValue = internal::serialize(statement, context);
            REQUIRE(stringValue == "(SELECT 1)");
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            auto stringValue = internal::serialize(statement, context);
            REQUIRE(stringValue == "SELECT 1");
        }
    }
    SECTION("compound operator") {
        auto statement = select(union_(select(1), select(2)));
        auto stringValue = internal::serialize(statement, context);
        REQUIRE(stringValue == "SELECT 1 UNION SELECT 2");
    }
    SECTION("columns") {
        auto statement = select(columns(1, 2));
        auto stringValue = internal::serialize(statement, context);
        SECTION("!highest_level") {
            statement.highest_level = false;
            auto stringValue = internal::serialize(statement, context);
            REQUIRE(stringValue == "(SELECT 1, 2)");
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            auto stringValue = internal::serialize(statement, context);
            REQUIRE(stringValue == "SELECT 1, 2");
        }
    }
}
