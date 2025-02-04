#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("issue1354") {
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

    struct UserStruct {
        uint32_t Id;
        std::string Name;
        std::string LastName;
    };

    constexpr auto userStruct = struct_<UserStruct>(&User::id, &User::name, &OtherUser::lastName);

    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id),
                                           make_column("name", &User::name),
                                           make_column("last_name", &User::lastName)),
                                make_table("other_users",
                                           make_column("id", &OtherUser::id),
                                           make_column("name", &OtherUser::name),
                                           make_column("last_name", &OtherUser::lastName)));
    storage.sync_schema();
    auto orderBySql = dynamic_order_by(storage);

    SECTION("name") {
        orderBySql.push_back(order_by(&User::name).asc());
    }
    SECTION("lastName") {
        orderBySql.push_back(order_by(&User::lastName).asc());
    }
    SECTION("both") {
        orderBySql.push_back(order_by(&User::name).asc());
        orderBySql.push_back(order_by(&User::lastName).asc());
    }

    auto users =
        storage.select(userStruct, left_join<OtherUser>(on(c(&User::id) == &OtherUser::id)), orderBySql, limit(1));
}
