#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer comparison operators") {
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
    SECTION("less_than") {
        SECTION("func") {
            value = serialize(less_than(4, 5), context);
        }
        SECTION("short func") {
            value = serialize(lt(4, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(4) < 5, context);
        }
        expected = "(4 < 5)";
    }
    SECTION("less_or_equal") {
        SECTION("func") {
            value = serialize(less_or_equal(10, 15), context);
        }
        SECTION("short func") {
            value = serialize(le(10, 15), context);
        }
        SECTION("operator") {
            value = serialize(c(10) <= 15, context);
        }
        expected = "(10 <= 15)";
    }
    SECTION("greater_than") {
        SECTION("func") {
            value = serialize(greater_than(1, 0.5), context);
        }
        SECTION("short func") {
            value = serialize(gt(1, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(1) > 0.5, context);
        }
        expected = "(1 > 0.5)";
    }
    SECTION("greater_or_equal") {
        SECTION("func") {
            value = serialize(greater_or_equal(10, -5), context);
        }
        SECTION("short func") {
            value = serialize(ge(10, -5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) >= -5, context);
        }
        expected = "(10 >= -5)";
    }
    SECTION("is_equal") {
        SECTION("func") {
            value = serialize(is_equal("ototo", "Hey"), context);
        }
        SECTION("short func") {
            value = serialize(eq("ototo", "Hey"), context);
        }
        SECTION("operator") {
            value = serialize(c("ototo") == "Hey", context);
        }
        expected = "('ototo' = 'Hey')";
    }
    SECTION("is_not_equal") {
        SECTION("func") {
            value = serialize(is_not_equal("lala", 7), context);
        }
        SECTION("short func") {
            value = serialize(ne("lala", 7), context);
        }
        SECTION("operator") {
            value = serialize(c("lala") != 7, context);
        }
        expected = "('lala' != 7)";
    }
    SECTION("is_equal_with_table_t") {
        value = serialize(is_equal<User>("Tom Gregory"), context);
        expected = R"("users" = 'Tom Gregory')";
    }
    SECTION("subquery") {
        context.use_parentheses = false;
        value = serialize(greater_than(&User::id, select(avg(&User::id))), context);
        expected = R"("id" > (SELECT (AVG("users"."id")) FROM "users"))";
    }
    REQUIRE(value == expected);
}
