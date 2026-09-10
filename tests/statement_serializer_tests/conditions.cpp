#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer conditions") {
    std::string value, expected;

    struct User {
        int64 id;
        std::string name;
    };

    auto t1 = make_table("user", make_column("id", &User::id), make_column("name", &User::name));
    auto storage = internal::db_objects_tuple<decltype(t1)>{t1};
    using db_objects_tuple = decltype(storage);

    internal::serializer_context<db_objects_tuple> ctx{storage};

    SECTION("using") {
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
        SECTION("positional ordinal") {
            auto expression = order_by(1);
            value = serialize(expression, ctx);
            expected = "ORDER BY 1";
        }
        SECTION("normal") {
            auto expression = order_by(&User::id);
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id")";
        }
        SECTION("asc") {
            auto expression = order_by(&User::id).asc();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC)";
        }
        SECTION("desc") {
            auto expression = order_by(&User::id).desc();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC)";
        }
#if SQLITE_VERSION_NUMBER >= 3030000
        SECTION("nulls first") {
            auto expression = order_by(&User::id).nulls_first();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" NULLS FIRST)";
        }
        SECTION("nulls last") {
            auto expression = order_by(&User::id).nulls_last();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" NULLS LAST)";
        }
        SECTION("asc, nulls first") {
            auto expression = order_by(&User::id).asc().nulls_first();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC NULLS FIRST)";
        }
        SECTION("asc, nulls last") {
            auto expression = order_by(&User::id).asc().nulls_last();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC NULLS LAST)";
        }
        SECTION("desc, nulls first") {
            auto expression = order_by(&User::id).desc().nulls_first();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC NULLS FIRST)";
        }
        SECTION("desc, nulls last") {
            auto expression = order_by(&User::id).desc().nulls_last();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC NULLS LAST)";
        }
        SECTION("the builder order does not matter") {
            auto expression = order_by(&User::id).nulls_last().desc();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC NULLS LAST)";
        }
        SECTION("collated, desc, nulls last") {
            auto expression = order_by(&User::name).collate_nocase().desc().nulls_last();
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."name" COLLATE NOCASE DESC NULLS LAST)";
        }
        SECTION("multi with mixed null placements") {
            auto expression =
                multi_order_by(order_by(&User::id).asc().nulls_last(), order_by(&User::name).desc().nulls_first());
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC NULLS LAST, "user"."name" DESC NULLS FIRST)";
        }
#endif
        SECTION("multi, single") {
            auto expression = multi_order_by(order_by(&User::id));
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id")";
        }
        SECTION("multi, single, asc") {
            auto expression = multi_order_by(order_by(&User::id).asc());
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC)";
        }
        SECTION("multi, single, desc") {
            auto expression = multi_order_by(order_by(&User::id).desc());
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC)";
        }
        SECTION("multi, several") {
            auto expression = multi_order_by(order_by(&User::id), order_by(&User::name));
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id", "user"."name")";
        }
        SECTION("multi, several, asc") {
            auto expression = multi_order_by(order_by(&User::id).asc(), order_by(&User::name).asc());
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" ASC, "user"."name" ASC)";
        }
        SECTION("multi, several, desc") {
            auto expression = multi_order_by(order_by(&User::id).desc(), order_by(&User::name).desc());
            value = serialize(expression, ctx);
            expected = R"(ORDER BY "user"."id" DESC, "user"."name" DESC)";
        }
    }

    REQUIRE(value == expected);
}
