#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#ifdef SQLITE_ORM_WITH_VIEW
using namespace sqlite_orm;

struct UserViewTests {
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
            make_view<UserViewTests>("user_view", select(asterisk<User>())));

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
            make_view<UserViewTests>("user_view",
                                     with(users_cte().as(select(asterisk<User>())), select(asterisk<users_cte>()))));

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
#endif
