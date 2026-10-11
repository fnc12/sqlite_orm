#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

/*
 *  The join operators SQLite offers, each with its constraint spelled out or left implicit,
 *  and NATURAL combined with each of them.
 */
TEST_CASE("join operators") {
    using internal::serialize;

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

    SECTION("implicit constraint") {
        SECTION("JOIN") {
            value = serialize(join<User>(), context);
            expected = R"(JOIN "users")";
        }
        SECTION("INNER JOIN") {
            value = serialize(inner_join<User>(), context);
            expected = R"(INNER JOIN "users")";
        }
        SECTION("LEFT JOIN") {
            value = serialize(left_join<User>(), context);
            expected = R"(LEFT JOIN "users")";
        }
        SECTION("LEFT OUTER JOIN") {
            value = serialize(left_outer_join<User>(), context);
            expected = R"(LEFT OUTER JOIN "users")";
        }
#if SQLITE_VERSION_NUMBER >= 3039000
        SECTION("RIGHT JOIN") {
            value = serialize(right_join<User>(), context);
            expected = R"(RIGHT JOIN "users")";
        }
        SECTION("RIGHT OUTER JOIN") {
            value = serialize(right_outer_join<User>(), context);
            expected = R"(RIGHT OUTER JOIN "users")";
        }
        SECTION("FULL JOIN") {
            value = serialize(full_join<User>(), context);
            expected = R"(FULL JOIN "users")";
        }
        SECTION("FULL OUTER JOIN") {
            value = serialize(full_outer_join<User>(), context);
            expected = R"(FULL OUTER JOIN "users")";
        }
#endif
    }
#if SQLITE_VERSION_NUMBER >= 3039000
    SECTION("explicit constraint") {
        SECTION("RIGHT JOIN") {
            value = serialize(right_join<User>(using_(&User::id)), context);
            expected = R"(RIGHT JOIN "users" USING ("id"))";
        }
        SECTION("RIGHT OUTER JOIN") {
            value = serialize(right_outer_join<User>(using_(&User::id)), context);
            expected = R"(RIGHT OUTER JOIN "users" USING ("id"))";
        }
        SECTION("FULL JOIN") {
            value = serialize(full_join<User>(using_(&User::id)), context);
            expected = R"(FULL JOIN "users" USING ("id"))";
        }
        SECTION("FULL OUTER JOIN") {
            value = serialize(full_outer_join<User>(using_(&User::id)), context);
            expected = R"(FULL OUTER JOIN "users" USING ("id"))";
        }
    }
#endif
    SECTION("NATURAL") {
        SECTION("NATURAL INNER JOIN") {
            value = serialize(natural_inner_join<User>(), context);
            expected = R"(NATURAL INNER JOIN "users")";
        }
        SECTION("NATURAL LEFT JOIN") {
            value = serialize(natural_left_join<User>(), context);
            expected = R"(NATURAL LEFT JOIN "users")";
        }
        SECTION("NATURAL LEFT OUTER JOIN") {
            value = serialize(natural_left_outer_join<User>(), context);
            expected = R"(NATURAL LEFT OUTER JOIN "users")";
        }
#if SQLITE_VERSION_NUMBER >= 3039000
        SECTION("NATURAL RIGHT JOIN") {
            value = serialize(natural_right_join<User>(), context);
            expected = R"(NATURAL RIGHT JOIN "users")";
        }
        SECTION("NATURAL RIGHT OUTER JOIN") {
            value = serialize(natural_right_outer_join<User>(), context);
            expected = R"(NATURAL RIGHT OUTER JOIN "users")";
        }
        SECTION("NATURAL FULL JOIN") {
            value = serialize(natural_full_join<User>(), context);
            expected = R"(NATURAL FULL JOIN "users")";
        }
        SECTION("NATURAL FULL OUTER JOIN") {
            value = serialize(natural_full_outer_join<User>(), context);
            expected = R"(NATURAL FULL OUTER JOIN "users")";
        }
#endif
        SECTION("alias") {
            using user_s = alias_s<User>;
            value = serialize(natural_left_join<user_s>(), context);
            expected = R"(NATURAL LEFT JOIN "users" "s")";
        }
    }
    REQUIRE(value == expected);
}
