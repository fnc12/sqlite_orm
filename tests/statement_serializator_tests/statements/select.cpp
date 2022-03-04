#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializator select_t") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using storage_impl_t = internal::storage_impl<decltype(table)>;
    storage_impl_t storageImpl{table};
    internal::serializator_context<storage_impl_t> context{storageImpl};
    std::string stringValue;
    decltype(stringValue) expected;
    SECTION("simple") {
        auto statement = select(1);
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = serialize(statement, context);
            expected = "(SELECT 1)";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = serialize(statement, context);
            expected = "SELECT 1";
        }
    }
    SECTION("row") {
        auto statement = select(is_equal(std::make_tuple(1, 2, 3), std::make_tuple(4, 5, 6)));
        SECTION("!highest_level") {
            statement.highest_level = false;
            stringValue = serialize(statement, context);
            expected = "(SELECT ((1, 2, 3) = (4, 5, 6)))";
        }
        SECTION("highest_level") {
            statement.highest_level = true;
            stringValue = serialize(statement, context);
            expected = "SELECT ((1, 2, 3) = (4, 5, 6))";
        }
    }
    SECTION("compound operator") {
        auto statement = select(union_(select(1), select(2)));
        stringValue = serialize(statement, context);
        expected = "SELECT 1 UNION SELECT 2";
    }
    SECTION("columns") {
        SECTION("literals") {
            auto statement = select(columns(1, 2));
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT 1, 2)";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT 1, 2";
            }
        }
        SECTION("from table") {
            auto statement = select(&User::id);
            SECTION("!highest_level") {
                statement.highest_level = false;
                stringValue = serialize(statement, context);
                expected = "(SELECT \"users\".\"id\" FROM 'users')";
            }
            SECTION("highest_level") {
                statement.highest_level = true;
                stringValue = serialize(statement, context);
                expected = "SELECT \"users\".\"id\" FROM 'users'";
            }
        }
    }
    REQUIRE(stringValue == expected);
}
