#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer conditions") {
    std::string value, expected;

    SECTION("using") {
        struct User {
            int64 id;
        };

        auto t1 = make_table("user", make_column("id", &User::id));
        auto storage = internal::db_objects_tuple<decltype(t1)>{t1};
        using db_objects_tuple = decltype(storage);

        internal::serializer_context<db_objects_tuple> ctx{storage};

        SECTION("using column") {
            auto expression = using_(&User::id);
            value = serialize(expression, ctx);
            expected = R"(USING ("id"))";
        }
        SECTION("using explicit column") {
            auto expression = using_(column<User>(&User::id));
            value = serialize(expression, ctx);
            expected = R"(USING ("id"))";
        }
    }
    SECTION("order by") {
        auto storage = internal::db_objects_tuple<>{};
        using db_objects_tuple = decltype(storage);

        internal::serializer_context<db_objects_tuple> ctx{storage};

        SECTION("positional ordinal") {
            auto expression = order_by(1);
            value = serialize(expression, ctx);
            expected = "ORDER BY 1";
        }
    }

    REQUIRE(value == expected);
}
