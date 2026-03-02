#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer is_null") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    std::string expected;

    SECTION("is_null with column") {
        value = serialize(is_null(&User::name), context);
        expected = R"("name" IS NULL)";
    }

    SECTION("is_not_null with column") {
        value = serialize(is_not_null(&User::name), context);
        expected = R"("name" IS NOT NULL)";
    }

    SECTION("is_null with negation") {
        value = serialize(!is_null(&User::name), context);
        expected = R"(NOT "name" IS NULL)";
    }

    SECTION("is_not_null with negation") {
        value = serialize(!is_not_null(&User::name), context);
        expected = R"(NOT "name" IS NOT NULL)";
    }

    SECTION("is_null in where clause") {
        context.use_parentheses = false;
        value = serialize(select(&User::id, where(is_null(&User::name))), context);
        expected = R"(SELECT "users"."id" FROM "users" WHERE ("users"."name" IS NULL))";
    }

    SECTION("is_not_null in where clause") {
        context.use_parentheses = false;
        value = serialize(select(&User::id, where(is_not_null(&User::name))), context);
        expected = R"(SELECT "users"."id" FROM "users" WHERE ("users"."name" IS NOT NULL))";
    }

    SECTION("is_null with and condition") {
        value = serialize(is_null(&User::name) and is_null(&User::id), context);
        expected = R"("name" IS NULL AND "id" IS NULL)";
    }

    SECTION("is_null with or condition") {
        value = serialize(is_null(&User::name) or is_not_null(&User::id), context);
        expected = R"("name" IS NULL OR "id" IS NOT NULL)";
    }

    REQUIRE(value == expected);
}
