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
    SECTION("from table") {
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
#ifdef SQLITE_ORM_WITH_CTE
    SECTION("from CTE") {
        auto dbObjects2 =
            internal::storage_db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cte<cte_1>()(select(1))));
        using context_t = internal::serializer_context<decltype(dbObjects2)>;
        context_t context{dbObjects2};
        SECTION("without alias 1") {
            auto expression = from<cte_1>();
            value = serialize(expression, context);
            expected = R"(FROM "1")";
        }
        SECTION("with alias 1") {
            auto expression = from<alias_z<cte_1>>();
            value = serialize(expression, context);
            expected = R"(FROM "1" "z")";
        }
#ifdef SQLITE_ORM_CLASSTYPE_TEMPLATE_ARG_SUPPORTED
        SECTION("without alias 2") {
            auto expression = from<"1"_cte>();
            value = serialize(expression, context);
            expected = R"(FROM "1")";
        }
        SECTION("with alias 2") {
            constexpr auto z_alias = "z"_alias("1"_cte);
            auto expression = from<z_alias>();
            value = serialize(expression, context);
            expected = R"(FROM "1" "z")";
        }
#endif
        SECTION("complete select") {
            auto expression = with(
                cte<cte_1>()(
                    union_all(select(1), select(1_ctealias->*1_colalias + c(1), where(1_ctealias->*1_colalias < 10)))),
                select(1_ctealias->*1_colalias));
            value = serialize(expression, context);
            expected =
                R"(WITH "1"("1") AS (SELECT 1 UNION ALL SELECT "1"."1" + 1 FROM "1" WHERE ("1"."1" < 10)) SELECT "1"."1" FROM "1")";
        }
    }
#endif
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
        // EXISTS must use parentheses
        context.use_parentheses = false;
        auto expression = exists(select(1));
        value = serialize(expression, context);
        expected = "EXISTS (SELECT 1)";
    }
    REQUIRE(value == expected);
}
