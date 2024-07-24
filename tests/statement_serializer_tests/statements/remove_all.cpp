#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer remove_all") {
    using internal::serialize;
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    db_objects_t dbObjects{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};

    std::string value;
    std::string expected;

    SECTION("all") {
        auto expression = remove_all<User>();
        value = serialize(expression, context);
        expected = R"(DELETE FROM "users")";
    }
    SECTION("where") {
        auto expression = remove_all<User>(where(&User::id == c(1)));
        value = serialize(expression, context);
        expected = R"(DELETE FROM "users" WHERE ("id" = 1))";
    }
    SECTION("conditions") {
        auto expression = remove_all<User>(where(&User::id == c(1)), limit(1));
        value = serialize(expression, context);
        expected = R"(DELETE FROM "users" WHERE ("id" = 1) LIMIT 1)";
    }
#if(SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
    SECTION("With clause") {
        constexpr orm_cte_moniker auto data = "data"_cte;
        constexpr auto cteExpression = cte<data>().as(select(1));
        auto dbObjects2 = internal::db_objects_cat(dbObjects, internal::make_cte_table(dbObjects, cteExpression));
        using context_t = internal::serializer_context<decltype(dbObjects2)>;
        context_t context2{dbObjects2};

        auto expression = with(cteExpression, remove_all<User>(where(c(&User::id).in(select(data->*1_colalias)))));
        value = serialize(expression, context2);
        expected =
            R"(WITH "data"("1") AS (SELECT 1) DELETE FROM "users" WHERE ("id" IN (SELECT "data"."1" FROM "data")))";
    }
#endif
#endif
    REQUIRE(value == expected);
}
