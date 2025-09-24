#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("dynamic_order_by") {
    using internal::serialize;

    struct User {
        uint32_t id;
        std::string name;
        std::string lastName;
    };

    struct OtherUser {
        uint32_t id;
        std::string name;
        std::string lastName;
    };

    auto table1 = make_table("users",
                             make_column("id", &User::id),
                             make_column("name", &User::name),
                             make_column("last_name", &User::lastName));
    auto table2 = make_table("other_users",
                             make_column("id", &OtherUser::id),
                             make_column("name", &OtherUser::name),
                             make_column("last_name", &OtherUser::lastName));

    using db_objects_t = internal::db_objects_tuple<decltype(table1), decltype(table2)>;
    auto dbObjects = db_objects_t{table1, table2};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    auto storage = make_storage("", table1, table2);

    std::string value;
    decltype(value) expected;

    auto orderBySql = dynamic_order_by(storage);
    SECTION("users.name") {
        orderBySql.push_back(order_by(&User::name).asc());
        expected = R"(ORDER BY "users"."name" ASC)";
    }
    SECTION("users.lastName") {
        orderBySql.push_back(order_by(&User::lastName).asc());
        expected = R"(ORDER BY "users"."last_name" ASC)";
    }
    SECTION("users.both") {
        orderBySql.push_back(order_by(&User::name).asc());
        orderBySql.push_back(order_by(&User::lastName).asc());
        expected = R"(ORDER BY "users"."name" ASC, "users"."last_name" ASC)";
    }
    SECTION("other_users.name") {
        orderBySql.push_back(order_by(&OtherUser::name).asc());
        expected = R"(ORDER BY "other_users"."name" ASC)";
    }
    SECTION("other_users.lastName") {
        orderBySql.push_back(order_by(&OtherUser::lastName).asc());
        expected = R"(ORDER BY "other_users"."last_name" ASC)";
    }
    SECTION("other_users.both") {
        orderBySql.push_back(order_by(&OtherUser::name).asc());
        orderBySql.push_back(order_by(&OtherUser::lastName).asc());
        expected = R"(ORDER BY "other_users"."name" ASC, "other_users"."last_name" ASC)";
    }
    value = serialize(orderBySql, context);

    REQUIRE(value == expected);
}
