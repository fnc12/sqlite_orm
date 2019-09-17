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
    
    remove("prepared.sqlite");
    auto storage = make_storage("prepared.sqlite",
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key(), autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time),
                                           foreign_key(&Visit::userId).references(&User::id)));
    storage.sync_schema();
    storage.remove_all<User>();
    
    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});
    
    SECTION("select") {
        {
            for(auto i = 0; i < 2; ++i) {
                auto statement = storage.prepare(select(&User::id));
                SECTION("nothing") {
                    //..
                }
                SECTION("exeecute") {
                    auto rows = storage.execute(statement);
                    REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
                }
            }
        }
        {
            for(auto i = 0; i < 2; ++i) {
                auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
                SECTION("nothing") {
                    //..
                }
                SECTION("exeecute") {
                    auto rows = storage.execute(statement);
                    REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
                }
            }
        }
        {
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
            }
        }
        {
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
            }
        }
        {
            auto statement = storage.prepare(select(columns(&User::id, &User::name)));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                std::vector<std::tuple<int, std::string>> expected;
                expected.push_back(std::make_tuple(1, "Team BS"));
                expected.push_back(std::make_tuple(2, "Shy'm"));
                expected.push_back(std::make_tuple(3, "Maître Gims"));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
        {
            auto statement = storage.prepare(select(columns(&User::name, &User::id), where(is_equal(mod(&User::id, 2), 0)),
                                                    order_by(&User::name)));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                std::vector<std::tuple<std::string, int>> expected;
                expected.push_back(std::make_tuple("Shy'm", 2));
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
    }
    SECTION("get all") {
        {
            auto statement = storage.prepare(get_all<User>());
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
        {
            auto statement = storage.prepare(get_all<User>(where(lesser_than(&User::id, 3))));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto rows = storage.execute(statement);
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
    }
    SECTION("update all") {
        auto statement = storage.prepare(update_all(set(assign(&User::name, conc(&User::name, "_")))));
        SECTION("nothing") {
            //..
        }
        SECTION("exeecute") {
            storage.execute(statement);
            auto names = storage.select(&User::name);
            std::vector<decltype(User::name)> expected;
            expected.push_back("Team BS_");
            expected.push_back("Shy'm_");
            expected.push_back("Maître Gims_");
            REQUIRE_THAT(names, UnorderedEquals(expected));
        }
    }
    SECTION("remove all") {
        auto statement = storage.prepare(remove_all<User>());
        SECTION("nothing") {
            //..
        }
        SECTION("exeecute") {
            storage.execute(statement);
            REQUIRE(storage.count<User>() == 0);
        }
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
    SECTION("get") {
        {
            auto statement = storage.prepare(get<User>(1));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{1, "Team BS"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(2));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{2, "Shy'm"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(3));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{3, "Maître Gims"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(4));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                try {
                    auto user = storage.execute(statement);
                    REQUIRE(false);
                } catch (const std::system_error &e) {
                    //..
                }
            }
        }
    }
    SECTION("get pointer") {
        {
            auto statement = storage.prepare(get_pointer<User>(1));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{1, "Team BS"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(2));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{2, "Shy'm"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(3));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{3, "Maître Gims"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(4));
            SECTION("nothing") {
                //..
            }
            SECTION("exeecute") {
                auto user = storage.execute(statement);
                REQUIRE(!user);
            }
        }
    }
    SECTION("update") {
        User user{2, "Stromae"};
        auto statement = storage.prepare(update(user));
        SECTION("nothing") {
            //..
        }
        SECTION("exeecute") {
            storage.execute(statement);
            REQUIRE(storage.get<User>(2) == user);
            auto names = storage.select(&User::name);
            REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
        }
    }
    SECTION("remove") {
        {
            auto statement = storage.prepare(remove<User>(1));
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) != nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 2);
        }
        {
            auto statement = storage.prepare(remove<User>(2));
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 1);
        }
        {
            auto statement = storage.prepare(remove<User>(3));
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) == nullptr);
            REQUIRE(storage.count<User>() == 0);
        }
    }
    SECTION("insert") {
        User user{0, "Stromae"};
        auto statement = storage.prepare(insert(user));
        SECTION("nothing") {
            //..
        }
        SECTION("exeecute") {
            auto insertedId = storage.execute(statement);
            {
                auto rows = storage.get_all<User>();
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                expected.push_back(User{3, "Maître Gims"});
                expected.push_back(User{4, user.name});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
            REQUIRE(insertedId == 4);
        }
    }
    SECTION("replace") {
        std::vector<User> expected;
        User user;
        SECTION("existing") {
            user = {1, "Stromae"};
            expected.push_back(User{1, "Stromae"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
        }
        SECTION("new") {
            user = {4, "Stromae"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(User{4, "Stromae"});
        }
        auto statement = storage.prepare(replace(user));
        storage.execute(statement);
        auto rows = storage.get_all<User>();
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    SECTION("insert range") {
        std::vector<User> users;
        std::vector<User> expected;
        expected.push_back(User{1, "Team BS"});
        expected.push_back(User{2, "Shy'm"});
        expected.push_back(User{3, "Maître Gims"});
        SECTION("empty"){
            try {
                auto statement = storage.prepare(insert_range(users.begin(), users.end()));
                REQUIRE(false);
            } catch (const std::system_error &e) {
                //..
            }
        }
        SECTION("one") {
            User user{4, "The Weeknd"};
            users.push_back(user);
            auto statement = storage.prepare(insert_range(users.begin(), users.end()));
            storage.execute(statement);
            expected.push_back(user);
        }
        SECTION("two") {
            User user1{4, "The Weeknd"};
            User user2{5, "Eva"};
            users.push_back(user1);
            users.push_back(user2);
            auto statement = storage.prepare(insert_range(users.begin(), users.end()));
            storage.execute(statement);
            expected.push_back(user1);
            expected.push_back(user2);
        }
        auto rows = storage.get_all<User>();
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
    SECTION("replace range") {
        std::vector<User> users;
        std::vector<User> expected;
        SECTION("empty") {
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            try {
                auto statement = storage.prepare(replace_range(users.begin(), users.end()));
                REQUIRE(false);
            } catch (const std::system_error &e) {
                //..
            }
        }
        SECTION("one existing") {
            User user{1, "Raye"};
            expected.push_back(user);
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            users.push_back(user);
            auto statement = storage.prepare(replace_range(users.begin(), users.end()));
            storage.execute(statement);
        }
        SECTION("one existing and one new") {
            User user{2, "Raye"};
            User user2{4, "Bebe Rexha"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(user);
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(user2);
            users.push_back(user);
            users.push_back(user2);
            auto statement = storage.prepare(replace_range(users.begin(), users.end()));
            storage.execute(statement);
        }
        SECTION("All existing") {
            users.push_back(User{1, "Selena Gomez"});
            users.push_back(User{2, "Polina"});
            users.push_back(User{3, "Polina"});
            expected = users;
            auto statement = storage.prepare(replace_range(users.begin(), users.end()));
            storage.execute(statement);
        }
        auto rows = storage.get_all<User>();
        REQUIRE_THAT(rows, UnorderedEquals(expected));
    }
}
