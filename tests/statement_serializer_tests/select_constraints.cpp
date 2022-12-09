#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer select constraints") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    internal::serializer_context<db_objects_t> context{dbObjects};

    std::string value;
    decltype(value) expected;
    SECTION("columns") {
        auto expression = columns(&User::id, &User::name);
        SECTION("use_parentheses") {
            context.use_parentheses = true;
            expected = R"(("id", "name"))";
        }
        SECTION("!use_parentheses") {
            context.use_parentheses = false;
            expected = R"("id", "name")";
        }
        value = serialize(expression, context);
    }
    SECTION("into") {
        auto expression = into<User>();
        value = serialize(expression, context);
        expected = R"(INTO "users")";
    }
    SECTION("insert constraint") {
        SECTION("abort") {
            auto expression = or_abort();
            value = serialize(expression, context);
            expected = "OR ABORT";
        }
        SECTION("fail") {
            auto expression = or_fail();
            value = serialize(expression, context);
            expected = "OR FAIL";
        }
        SECTION("ignore") {
            auto expression = or_ignore();
            value = serialize(expression, context);
            expected = "OR IGNORE";
        }
        SECTION("replace") {
            auto expression = or_replace();
            value = serialize(expression, context);
            expected = "OR REPLACE";
        }
        SECTION("rollback") {
            auto expression = or_rollback();
            value = serialize(expression, context);
            expected = "OR ROLLBACK";
        }
    }
    SECTION("from") {
        SECTION("without alias") {
            auto expression = from<User>();
            value = serialize(expression, context);
            expected = R"(FROM "users")";
        }
        SECTION("with alias") {
            auto expression = from<alias_u<User>>();
            value = serialize(expression, context);
            expected = R"(FROM "users" "u")";
        }
    }
    SECTION("function_call") {
        struct Func {
            bool operator()(int arg) const {
                return arg % 2;
            }

            static const char* name() {
                return "EVEN";
            }
        };
        auto expression = func<Func>(&User::id);
        value = serialize(expression, context);
        expected = R"(EVEN("id"))";
    }
    REQUIRE(value == expected);
}
