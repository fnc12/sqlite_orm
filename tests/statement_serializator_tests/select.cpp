#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator select_t") {
    using storage_impl_t = internal::storage_impl<>;
    storage_impl_t storageImpl;
    internal::serializator_context<storage_impl_t> context{storageImpl};
    std::string stringValue;
    decltype(stringValue) expected;
    SECTION("simple") {
        auto statement = select(1);
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = internal::serialize(statement, context);
            expected = "(SELECT 1)";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = internal::serialize(statement, context);
            expected = "SELECT 1";
        }
    }
    SECTION("row") {
        auto statement = select(is_equal(std::make_tuple(1, 2, 3), std::make_tuple(4, 5, 6)));
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = internal::serialize(statement, context);
            expected = "(SELECT ((1, 2, 3) = (4, 5, 6)))";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = internal::serialize(statement, context);
            expected = "SELECT ((1, 2, 3) = (4, 5, 6))";
        }
    }
    SECTION("compound operator") {
        auto statement = select(union_(select(1), select(2)));
        stringValue = internal::serialize(statement, context);
        expected = "SELECT 1 UNION SELECT 2";
    }
    SECTION("columns") {
        auto statement = select(columns(1, 2));
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = internal::serialize(statement, context);
            expected = "(SELECT 1, 2)";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = internal::serialize(statement, context);
            expected = "SELECT 1, 2";
        }
    }
    REQUIRE(stringValue == expected);
}
