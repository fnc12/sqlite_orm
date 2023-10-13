#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer column") {
    struct User {
        int id = 0;
        std::string name;
        std::unique_ptr<std::string> nullableText;
    };
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    std::string expected;
    SECTION("with types and constraints") {
        context.skip_types_and_constraints = false;
        SECTION("id INTEGER (implicit) NOT NULL") {
            auto column = make_column("id", &User::id);
            value = serialize(column, context);
            expected = "\"id\" INTEGER NOT NULL";
        }
        SECTION("id INTEGER (explicit) NOT NULL") {
            auto column = make_column("id", &User::id, not_null());
            value = serialize(column, context);
            expected = "\"id\" INTEGER NOT NULL";
        }
        SECTION("id INTEGER (explicit) NULL") {
            auto column = make_column("id", &User::id, null());
            value = serialize(column, context);
            expected = "\"id\" INTEGER NULL";
        }
        SECTION("name TEXT (implicit) NOT NULL") {
            auto column = make_column("name", &User::name);
            value = serialize(column, context);
            expected = "\"name\" TEXT NOT NULL";
        }
        SECTION("name TEXT (explicit) NOT NULL") {
            auto column = make_column("name", &User::name, not_null());
            value = serialize(column, context);
            expected = "\"name\" TEXT NOT NULL";
        }
        SECTION("name TEXT (explicit) NULL") {
            auto column = make_column("name", &User::name, null());
            value = serialize(column, context);
            expected = "\"name\" TEXT NULL";
        }
        SECTION("nullable text (implicit) NULL") {
            auto column = make_column("nullable_text", &User::nullableText);
            value = serialize(column, context);
            expected = "\"nullable_text\" TEXT NULL";
        }
        SECTION("nullable text (explicit) NOT NULL") {
            auto column = make_column("nullable_text", &User::nullableText, not_null());
            value = serialize(column, context);
            expected = "\"nullable_text\" TEXT NOT NULL";
        }
        SECTION("nullable text (explicit) NULL") {
            auto column = make_column("nullable_text", &User::nullableText, null());
            value = serialize(column, context);
            expected = "\"nullable_text\" TEXT NULL";
        }
    }
    SECTION("without types and constraints") {
        context.skip_types_and_constraints = true;
        SECTION("id INTEGER NOT NULL") {
            auto column = make_column("id", &User::id);
            value = serialize(column, context);
            expected = "\"id\"";
        }
        SECTION("name TEXT NOT NULL") {
            auto column = make_column("name", &User::name);
            value = serialize(column, context);
            expected = "\"name\"";
        }
    }
    REQUIRE(value == expected);
}
