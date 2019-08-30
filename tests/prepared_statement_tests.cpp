#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

struct User {
    int id = 0;
    std::string name;
};

struct Visit {
    int id = 0;
    decltype(User::id) userId;
    long time = 0;
};

bool operator==(const User &lhs, const User &rhs) {
    return lhs.id == rhs.id && lhs.name == rhs.name;
}

bool operator!=(const User &lhs, const User &rhs) {
    return !(lhs == rhs);
}

TEST_CASE("Prepared") {
    using Catch::Matchers::UnorderedEquals;
    
    auto openCount = 0;
    
    remove("prepared.sqlite");
    auto storage = make_storage("prepared.sqlite",
//                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key(), autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time),
                                           foreign_key(&Visit::userId).references(&User::id)));
    storage.on_open = [&openCount] (sqlite3 *){
        ++openCount;
    };
    storage.sync_schema();
    storage.remove_all<User>();
    
    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});
    
    SECTION("select") {
        {
            for(auto i = 0; i < 2; ++i) {
                auto guard = storage.transaction_guard();
                auto statement = storage.prepare(select(&User::id));
                auto rows = storage.execute(statement);
                guard.commit();
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
            }
        }
        {
            for(auto i = 0; i < 2; ++i) {
                auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
            }
        }
        {
            auto guard = storage.transaction_guard();
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
            guard.commit();
        }
        {
            auto guard = storage.transaction_guard();
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
            auto rows = storage.execute(statement);
            REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
            guard.commit();
        }
        {
            auto guard = storage.transaction_guard();
            auto statement = storage.prepare(select(columns(&User::id, &User::name)));
            auto rows = storage.execute(statement);
            std::vector<std::tuple<int, std::string>> expected;
            expected.push_back(std::make_tuple(1, "Team BS"));
            expected.push_back(std::make_tuple(2, "Shy'm"));
            expected.push_back(std::make_tuple(3, "Maître Gims"));
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            guard.commit();
        }
        {
            auto guard = storage.transaction_guard();
            auto statement = storage.prepare(select(columns(&User::name, &User::id), where(is_equal(mod(&User::id, 2), 0)),
                                                    order_by(&User::name)));
            auto rows = storage.execute(statement);
            std::vector<std::tuple<std::string, int>> expected;
            expected.push_back(std::make_tuple("Shy'm", 2));
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            guard.commit();
        }
    }
    SECTION("get all") {
        {
            auto guard = storage.transaction_guard();
            auto oCount = openCount;
            auto statement = storage.prepare(get_all<User>());
            auto rows = storage.execute(statement);
            std::vector<User> expected;
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            guard.commit();
            REQUIRE(openCount == oCount);
        }
        {
            auto guard = storage.transaction_guard();
            auto statement = storage.prepare(get_all<User>(where(lesser_than(&User::id, 3))));
            auto rows = storage.execute(statement);
            std::vector<User> expected;
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            guard.commit();
        }
    }
    SECTION("update all") {
        auto guard = storage.transaction_guard();
        auto statement = storage.prepare(update_all(set(assign(&User::name, conc(&User::name, "_")))));
        storage.execute(statement);
        auto names = storage.select(&User::name);
        std::vector<decltype(User::name)> expected;
        expected.push_back("Team BS_");
        expected.push_back("Shy'm_");
        expected.push_back("Maître Gims_");
        REQUIRE_THAT(names, UnorderedEquals(expected));
        guard.commit();
    }
    SECTION("remove all") {
        auto guard = storage.transaction_guard();
        auto statement = storage.prepare(remove_all<User>());
        storage.execute(statement);
        guard.commit();
        REQUIRE(storage.count<User>() == 0);
    }
    SECTION("remove all 2") {
        SECTION("One condition") {
            auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::id, 2))));
            storage.execute(statement);
        }
        SECTION("Two conditions") {
            auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::name, "Shy'm") and lesser_than(&User::id, 10))));
            storage.execute(statement);
        }
        auto ids = storage.select(&User::id);
        decltype(ids) expected;
        expected.push_back(1);
        expected.push_back(3);
        REQUIRE_THAT(ids, UnorderedEquals(expected));
    }
}
