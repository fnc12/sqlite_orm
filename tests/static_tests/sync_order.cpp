#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;
    };
    struct Visit {
        int id = 0;
        int userId = 0;
    };
}

TEST_CASE("sync_schema dependency order is computed at compile time") {
    using UsersTable =
        decltype(make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    using VisitsTable = decltype(make_table("visits",
                                            make_column("id", &Visit::id, primary_key()),
                                            make_column("user_id", &Visit::userId)));
    using UsersIndex = decltype(make_index("idx_users_name", &User::name));
    using VisitsIndex = decltype(make_index("idx_visits_user_id", &Visit::userId));
    using UsersTrigger =
        decltype(make_trigger("trg_users",
                              after().insert().on<User>().begin(update_all(set(c(&User::name) = "updated")))));

    SECTION("objects without dependencies keep the declaration order") {
        using Objects = internal::db_objects_tuple<UsersTable, VisitsTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<0, 1>>::value);
    }
    SECTION("a straight declaration order is kept") {
        using Objects = internal::db_objects_tuple<UsersTable, UsersIndex>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<0, 1>>::value);
    }
    SECTION("an index declared before its table is deferred") {
        using Objects = internal::db_objects_tuple<UsersIndex, UsersTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<1, 0>>::value);
    }
    SECTION("a trigger declared before its table is deferred") {
        using Objects = internal::db_objects_tuple<UsersTrigger, UsersTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<1, 0>>::value);
    }
    SECTION("a fan of dependent objects follows its table, keeping the declaration order among themselves") {
        using Objects = internal::db_objects_tuple<UsersIndex, UsersTrigger, UsersTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<2, 0, 1>>::value);
    }
    SECTION("interleaved objects are deferred only until their own table") {
        using Objects = internal::db_objects_tuple<VisitsIndex, UsersTable, UsersIndex, VisitsTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<1, 2, 3, 0>>::value);
    }
    SECTION("an index waits for all tables mapping the same type") {
        //  two tables may map the same C++ type under different names;
        //  the dependency is computed by type, so the index conservatively follows both
        using Objects = internal::db_objects_tuple<UsersTable, UsersIndex, UsersTable>;
        STATIC_REQUIRE(std::is_same<internal::sync_order_sequence_t<Objects>, std::index_sequence<0, 2, 1>>::value);
    }
}
