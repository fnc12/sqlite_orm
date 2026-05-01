#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdio>  //  std::remove

#ifdef SQLITE_ORM_WITH_VIEW
using namespace sqlite_orm;

struct[[= dbo_name("user_view")]] UserViewTests {
    int id = 0;
    std::string name;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
    bool operator==(const UserViewTests&) const = default;
#else
    bool operator==(const UserViewTests& right) const {
        return id == right.id && name == right.name;
    }
#endif
};

struct[[= dbo_name("user_view")]] UserView2Tests {
    std::string name;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
    bool operator==(const UserView2Tests&) const = default;
#else
    bool operator==(const UserView2Tests& right) const {
        return name == right.name;
    }
#endif
};

TEST_CASE("sql view") {
    using Catch::Matchers::UnorderedEquals;

    struct User {
        int id = 0;
        std::string name;
    };

    SECTION("normal") {
        auto storage = make_storage(
            "",
            make_table<User>("user", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_view<UserViewTests>(select(asterisk<User>())));

        storage.sync_schema();

        storage.transaction([&storage] {
            storage.insert<User>({0, "name"});
            return true;
        });

        SECTION("created view") {
            auto viewNames = storage.view_names();
            REQUIRE_THAT(viewNames, UnorderedEquals<std::string>({"user_view"}));
        }
        SECTION("view select") {
            auto users = storage.select(object<UserViewTests>());
            REQUIRE_THAT(users, UnorderedEquals<UserViewTests>({{1, "name"}}));
        }
    }
    SECTION("view with CTE") {
#if (SQLITE_VERSION_NUMBER >= 3008003) && defined(SQLITE_ORM_WITH_CTE)
#ifdef SQLITE_ORM_WITH_CPP20_ALIASES
        constexpr orm_cte_moniker auto users_cte = "users"_cte;
        auto storage = make_storage(
            "",
            make_table<User>("user", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_view<UserViewTests>(with(users_cte().as(select(asterisk<User>())), select(asterisk<users_cte>()))));

        storage.sync_schema();

        storage.transaction([&storage] {
            storage.insert<User>({0, "name"});
            return true;
        });

        SECTION("created view") {
            auto viewNames = storage.view_names();
            REQUIRE_THAT(viewNames, UnorderedEquals<std::string>({"user_view"}));
        }
        SECTION("view select") {
            auto users = storage.select(object<UserViewTests>());
            REQUIRE_THAT(users, UnorderedEquals<UserViewTests>({{1, "name"}}));
        }
#endif
#endif
    }
}

TEST_CASE("sync sql view") {
    struct User {
        int id = 0;
        std::string name;
    };

    auto storagePath = "sync_sql_view.sqlite";
    std::remove(storagePath);

    // first: create storage with trigger checking "name" column
    {
        auto storage = make_storage(
            storagePath,
            make_table<User>("user", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_view<UserView2Tests>(select(&User::name)));
        auto syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("user_view") == sync_schema_result::new_table_created);

        // second sync should report already_in_sync
        syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("user_view") == sync_schema_result::already_in_sync);
    }
    // second: create storage with a different view on User instead
    {
        auto storage = make_storage(
            storagePath,
            make_table<User>("user", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_view<UserViewTests>(select(asterisk<User>())));

        // simulate should detect the change
        auto simulateResult = storage.sync_schema_simulate();
        REQUIRE(simulateResult.at("user_view") == sync_schema_result::dropped_and_recreated);

        // sync should update the view
        auto syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("user_view") == sync_schema_result::dropped_and_recreated);

        // verify view was updated
        REQUIRE_NOTHROW(storage.iterate<UserViewTests>());

        // after update, second sync should be already_in_sync
        syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("user_view") == sync_schema_result::already_in_sync);
    }

    std::remove(storagePath);
}
#endif
