#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Prepare select") {
    using Catch::Matchers::UnorderedEquals;
    
    struct User {
        int id = 0;
        std::string name;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});
    {
        auto statement = storage.prepare(select(&User::id));
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
    }
    {
        auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
    }
    {
        auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
    }
    {
        auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
        auto rows = storage.execute(statement);
        REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
    }
    {
        auto statement = storage.prepare(select(columns(&User::id, &User::name)));
        auto rows = storage.execute(statement);
        std::vector<std::tuple<int, std::string>> expected = {
            {1, "Team BS"},
            {2, "Shy'm"},
            {3, "Maître Gims"},
        };
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    {
        auto statement = storage.prepare(select(columns(&User::name, &User::id), where(is_equal(mod(&User::id, 2), 0)),
                                                order_by(&User::name)));
        auto rows = storage.execute(statement);
        std::vector<std::tuple<std::string, int>> expected = {
            {"Shy'm", 2},
        };
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}
