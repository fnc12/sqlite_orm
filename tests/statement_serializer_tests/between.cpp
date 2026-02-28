#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer between") {
    struct User {
        int id = 0;
        std::string name;
        int age = 0;
    };
    auto table = make_table("users",
                            make_column("id", &User::id),
                            make_column("name", &User::name),
                            make_column("age", &User::age));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string value;
    std::string expected;

    SECTION("between with column and literal values") {
        SECTION("integer range") {
            value = serialize(between(&User::id, 1, 10), context);
            expected = R"("id" BETWEEN 1 AND 10)";
        }
        SECTION("age range") {
            value = serialize(between(&User::age, 18, 65), context);
            expected = R"("age" BETWEEN 18 AND 65)";
        }
    }

    SECTION("between with cast") {
        value = serialize(between(cast<int>(&User::name), 1, 5), context);
        expected = R"(CAST ("name" AS INTEGER) BETWEEN 1 AND 5)";
    }

    SECTION("between with function") {
        value = serialize(between(length(&User::name), 5, 20), context);
        expected = R"(LENGTH("name") BETWEEN 5 AND 20)";
    }

    SECTION("between in where clause") {
        context.use_parentheses = false;
        value = serialize(select(object<User>(), where(between(&User::age, 18, 65))), context);
        expected = R"(SELECT "users".* FROM "users" WHERE ("users"."age" BETWEEN 18 AND 65))";
    }

    SECTION("between with and condition") {
        value = serialize(between(&User::id, 5, 15) and between(&User::age, 20, 30), context);
        expected = R"("id" BETWEEN 5 AND 15 AND "age" BETWEEN 20 AND 30)";
    }

    SECTION("between with or condition") {
        value = serialize(between(&User::id, 1, 5) or between(&User::id, 15, 20), context);
        expected = R"("id" BETWEEN 1 AND 5 OR "id" BETWEEN 15 AND 20)";
    }

    SECTION("between with negation") {
        value = serialize(!between(&User::age, 0, 18), context);
        expected = R"(NOT "age" BETWEEN 0 AND 18)";
    }

    REQUIRE(value == expected);
}
