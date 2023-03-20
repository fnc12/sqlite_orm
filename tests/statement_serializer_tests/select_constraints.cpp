#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

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
    // tests whether the statement serializer for a select with joins
    // properly deduplicates the table names when no explicit from is used
    SECTION("deduplicated table names") {
        struct UserProps {
            int id = 0;
            std::string name;
        };
        auto table2 =
            make_table("user_props", make_column("id", &UserProps::id), make_column("name", &UserProps::name));
        using db_objects_t = internal::db_objects_tuple<decltype(table), decltype(table2)>;
        auto dbObjects = db_objects_t{table, table2};
        internal::serializer_context<db_objects_t> context{dbObjects};
        context.use_parentheses = false;

        SECTION("left join") {
            auto expression = select(asterisk<User>(), left_join<UserProps>(using_(&UserProps::id)));
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" LEFT JOIN "user_props" USING ("id"))";
        }
        SECTION("join") {
            auto expression = select(asterisk<User>(), join<UserProps>(using_(&UserProps::id)));
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" JOIN "user_props" USING ("id"))";
        }
        SECTION("left outer join") {
            auto expression = select(asterisk<User>(), left_outer_join<UserProps>(using_(&UserProps::id)));
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" LEFT OUTER JOIN "user_props" USING ("id"))";
        }
        SECTION("inner join") {
            auto expression = select(asterisk<User>(), inner_join<UserProps>(using_(&UserProps::id)));
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" INNER JOIN "user_props" USING ("id"))";
        }
        SECTION("cross join") {
            auto expression = select(asterisk<User>(), cross_join<UserProps>());
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" CROSS JOIN "user_props")";
        }
        SECTION("natural join") {
            auto expression = select(asterisk<User>(), natural_join<UserProps>());
            value = serialize(expression, context);
            expected = R"(SELECT "users".* FROM "users" NATURAL JOIN "user_props")";
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
    SECTION("exists") {
        // EXISTS must use parentheses in a new context
        context.use_parentheses = false;
        auto expression = exists(select(1));
        value = serialize(expression, context);
        expected = "EXISTS (SELECT 1)";
    }
    REQUIRE(value == expected);
}
