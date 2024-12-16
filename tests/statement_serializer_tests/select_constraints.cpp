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
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        {
            SECTION("with table reference") {
                constexpr auto user = c<User>();
                auto expression = from<user>();
                value = serialize(expression, context);
                expected = R"(FROM "users")";
            }
            SECTION("with alias 2") {
                auto expression = from<alias<'u'>.for_<User>()>();
                value = serialize(expression, context);
                expected = R"(FROM "users" "u")";
            }
            SECTION("with alias 3") {
                auto expression = from<"u"_alias.for_<User>()>();
                value = serialize(expression, context);
                expected = R"(FROM "users" "u")";
            }
        }
#endif
    }
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
    SECTION("from CTE") {
        using cte_1 = decltype(1_ctealias);
        auto dbObjects2 =
            internal::db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cte<cte_1>().as(select(1))));
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
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        SECTION("without alias 2") {
            auto expression = from<1_ctealias>();
            value = serialize(expression, context);
            expected = R"(FROM "1")";
        }
        SECTION("with alias 2") {
            constexpr auto z_alias = "z"_alias.for_<1_ctealias>();
            auto expression = from<z_alias>();
            value = serialize(expression, context);
            expected = R"(FROM "1" "z")";
        }
        SECTION("as") {
            auto expression = cte<cte_1>().as(select(1));
            value = serialize(expression, context);
            expected = R"("1"("1") AS (SELECT 1))";
        }
#if SQLITE_VERSION_NUMBER >= 3035000
        SECTION("as materialized") {
            auto expression = cte<cte_1>().as<materialized()>(select(1));
            value = serialize(expression, context);
            expected = R"("1"("1") AS MATERIALIZED (SELECT 1))";
        }
        SECTION("as not materialized") {
            auto expression = cte<cte_1>().as<not_materialized()>(select(1));
            value = serialize(expression, context);
            expected = R"("1"("1") AS NOT MATERIALIZED (SELECT 1))";
        }
#endif
#endif
        SECTION("with ordinary") {
            auto expression = with(cte<cte_1>().as(select(1)), select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected = R"(WITH "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1")";
        }
        SECTION("with ordinary, compound") {
            auto expression = with(cte<cte_1>().as(select(1)),
                                   union_all(select(column<cte_1>(1_colalias)), select(column<cte_1>(1_colalias))));
            value = serialize(expression, context);
            expected = R"(WITH "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1" UNION ALL SELECT "1"."1" FROM "1")";
        }
        SECTION("with not enforced recursive") {
            auto expression = with_recursive(cte<cte_1>().as(select(1)), select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected = R"(WITH RECURSIVE "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1")";
        }
        SECTION("with not enforced recursive, compound") {
            auto expression =
                with_recursive(cte<cte_1>().as(select(1)),
                               union_all(select(column<cte_1>(1_colalias)), select(column<cte_1>(1_colalias))));
            value = serialize(expression, context);
            expected =
                R"(WITH RECURSIVE "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1" UNION ALL SELECT "1"."1" FROM "1")";
        }
        SECTION("with ordinary, multiple") {
            auto expression = with(std::make_tuple(cte<cte_1>().as(select(1)), cte<cte_1>().as(select(1))),
                                   select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected = R"(WITH "1"("1") AS (SELECT 1), "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1")";
        }
        SECTION("with ordinary, multiple, compound") {
            auto expression = with(std::make_tuple(cte<cte_1>().as(select(1)), cte<cte_1>().as(select(1))),
                                   union_all(select(column<cte_1>(1_colalias)), select(column<cte_1>(1_colalias))));
            value = serialize(expression, context);
            expected =
                R"(WITH "1"("1") AS (SELECT 1), "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1" UNION ALL SELECT "1"."1" FROM "1")";
        }
        SECTION("with not enforced recursive, multiple") {
            auto expression = with_recursive(std::make_tuple(cte<cte_1>().as(select(1)), cte<cte_1>().as(select(1))),
                                             select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected = R"(WITH RECURSIVE "1"("1") AS (SELECT 1), "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1")";
        }
        SECTION("with not enforced recursive, multiple, compound") {
            auto expression =
                with_recursive(std::make_tuple(cte<cte_1>().as(select(1)), cte<cte_1>().as(select(1))),
                               union_all(select(column<cte_1>(1_colalias)), select(column<cte_1>(1_colalias))));
            value = serialize(expression, context);
            expected =
                R"(WITH RECURSIVE "1"("1") AS (SELECT 1), "1"("1") AS (SELECT 1) SELECT "1"."1" FROM "1" UNION ALL SELECT "1"."1" FROM "1")";
        }
        SECTION("with optional recursive") {
            auto expression = with(
                cte<cte_1>().as(
                    union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
                select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected =
                R"(WITH "1"("1") AS (SELECT 1 UNION ALL SELECT "1"."1" + 1 FROM "1" WHERE ("1"."1" < 10)) SELECT "1"."1" FROM "1")";
        }
        SECTION("with recursive") {
            auto expression = with_recursive(
                cte<cte_1>().as(
                    union_all(select(1), select(column<cte_1>(1_colalias) + 1, where(column<cte_1>(1_colalias) < 10)))),
                select(column<cte_1>(1_colalias)));
            value = serialize(expression, context);
            expected =
                R"(WITH RECURSIVE "1"("1") AS (SELECT 1 UNION ALL SELECT "1"."1" + 1 FROM "1" WHERE ("1"."1" < 10)) SELECT "1"."1" FROM "1")";
        }
    }
#endif
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
        expected = R"("EVEN"("id"))";
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
