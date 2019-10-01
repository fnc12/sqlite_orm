#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <tuple>    //  std::ignore

using namespace sqlite_orm;

namespace PreparedStatementTests {
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
    
    void testSerializing(const internal::prepared_statement_base &statement) {
        auto sql = statement.sql();
        std::ignore = sql;
        auto expanded = statement.expanded_sql();
        std::ignore = expanded;
#if SQLITE_VERSION_NUMBER >= 3027000
        auto normalized = statement.normalized_sql();
        std::ignore = normalized;
#endif
    }
}

TEST_CASE("Prepared") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;
    
    const int defaultVisitTime = 50;
    
    remove("prepared.sqlite");
    auto storage = make_storage("prepared.sqlite",
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key(), autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key(), autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time, default_value(defaultVisitTime)),
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
                testSerializing(statement);
                SECTION("nothing") {
                    //..
                }
                SECTION("execute") {
                    auto rows = storage.execute(statement);
                    REQUIRE_THAT(rows, UnorderedEquals<int>({1, 2, 3}));
                }
            }
        }
        {
            for(auto i = 0; i < 2; ++i) {
                auto statement = storage.prepare(select(&User::name, order_by(&User::id)));
                testSerializing(statement);
                SECTION("nothing") {
                    //..
                }
                SECTION("execute") {
                    auto rows = storage.execute(statement);
                    REQUIRE_THAT(rows, UnorderedEquals<std::string>({"Team BS", "Shy'm", "Maître Gims"}));
                }
            }
        }
        {
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5)));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1, 3}));
            }
        }
        {
            auto statement = storage.prepare(select(&User::id, where(length(&User::name) > 5 and like(&User::name, "T%"))));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto rows = storage.execute(statement);
                REQUIRE_THAT(rows, UnorderedEquals<int>({1}));
            }
        }
        {
            auto statement = storage.prepare(select(columns(&User::id, &User::name)));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
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
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
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
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
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
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
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
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
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
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            storage.execute(statement);
            REQUIRE(storage.count<User>() == 0);
        }
    }
    SECTION("remove all 2") {
        SECTION("One condition") {
            auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::id, 2))));
            testSerializing(statement);
            storage.execute(statement);
        }
        SECTION("Two conditions") {
            auto statement = storage.prepare(remove_all<User>(where(is_equal(&User::name, "Shy'm") and lesser_than(&User::id, 10))));
            testSerializing(statement);
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
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{1, "Team BS"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(2));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{2, "Shy'm"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(3));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user == User{3, "Maître Gims"});
            }
        }
        {
            auto statement = storage.prepare(get<User>(4));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
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
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{1, "Team BS"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(2));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{2, "Shy'm"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(3));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(user);
                REQUIRE(*user == User{3, "Maître Gims"});
            }
        }
        {
            auto statement = storage.prepare(get_pointer<User>(4));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto user = storage.execute(statement);
                REQUIRE(!user);
            }
        }
    }
    SECTION("update") {
        User user{2, "Stromae"};
        SECTION("by ref") {
            auto statement = storage.prepare(update(user));
            REQUIRE(get<0>(statement) == user);
            std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
            REQUIRE(&get<0>(statement) == &user);
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                storage.execute(statement);
                REQUIRE(storage.get<User>(2) == user);
                {
                    auto names = storage.select(&User::name);
                    REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                    REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
                }
                user.name = "Sia";
                storage.execute(statement);
                REQUIRE(storage.get<User>(2) == user);
                {
                    auto names = storage.select(&User::name);
                    REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                    REQUIRE(find(names.begin(), names.end(), "Sia") != names.end());
                }
            }
        }
        SECTION("by val") {
            auto statement = storage.prepare(update<by_val<User>>(user));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                storage.execute(statement);
                REQUIRE(storage.get<User>(2) == user);
                {
                    auto names = storage.select(&User::name);
                    REQUIRE(find(names.begin(), names.end(), "Shy'm") == names.end());
                    REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
                }
                
                //  try to change original user's name and perform the query. We expect that nothing will change
                user.name = "Sia";
                storage.execute(statement);
                {
                    auto names = storage.select(&User::name);
                    REQUIRE(find(names.begin(), names.end(), "Sia") == names.end());
                    REQUIRE(find(names.begin(), names.end(), "Stromae") != names.end());
                }
                
                //  now let's change statement's user's name. This time it musk work!
                get<0>(statement).name = "Sia";
                storage.execute(statement);
                REQUIRE(storage.count<User>(where(is_equal(&User::name, "Sia"))) == 1);
            }
        }
    }
    SECTION("remove") {
        {
            auto statement = storage.prepare(remove<User>(1));
            testSerializing(statement);
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) != nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 2);
        }
        {
            auto statement = storage.prepare(remove<User>(2));
            testSerializing(statement);
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) != nullptr);
            REQUIRE(storage.count<User>() == 1);
        }
        {
            auto statement = storage.prepare(remove<User>(3));
            testSerializing(statement);
            storage.execute(statement);
            REQUIRE(storage.get_pointer<User>(1) == nullptr);
            REQUIRE(storage.get_pointer<User>(2) == nullptr);
            REQUIRE(storage.get_pointer<User>(3) == nullptr);
            REQUIRE(storage.count<User>() == 0);
        }
    }
    SECTION("insert") {
        User user{0, "Stromae"};
        SECTION("by ref") {
            auto statement = storage.prepare(insert(user));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto insertedId = storage.execute(statement);
                {
                    auto rows = storage.get_all<User>();
                    std::vector<User> expected;
                    expected.push_back(User{1, "Team BS"});
                    expected.push_back(User{2, "Shy'm"});
                    expected.push_back(User{3, "Maître Gims"});
                    expected.push_back(User{4, "Stromae"});
                    REQUIRE_THAT(rows, UnorderedEquals(expected));
                }
                REQUIRE(insertedId == 4);
                user.name = "Sia";
                std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
                REQUIRE(get<0>(statement) == user);
                REQUIRE(&get<0>(statement) == &user);
                insertedId = storage.execute(statement);
                {
                    auto rows = storage.get_all<User>();
                    std::vector<User> expected;
                    expected.push_back(User{1, "Team BS"});
                    expected.push_back(User{2, "Shy'm"});
                    expected.push_back(User{3, "Maître Gims"});
                    expected.push_back(User{4, "Stromae"});
                    expected.push_back(User{5, "Sia"});
                    REQUIRE_THAT(rows, UnorderedEquals(expected));
                }
                REQUIRE(insertedId == 5);
            }
        }
        SECTION("by val") {
            auto statement = storage.prepare(insert<by_val<User>>(user));
            testSerializing(statement);
            SECTION("nothing") {
                //..
            }
            SECTION("execute") {
                auto insertedId = storage.execute(statement);
                {
                    auto rows = storage.get_all<User>();
                    std::vector<User> expected;
                    expected.push_back(User{1, "Team BS"});
                    expected.push_back(User{2, "Shy'm"});
                    expected.push_back(User{3, "Maître Gims"});
                    expected.push_back(User{4, "Stromae"});
                    REQUIRE_THAT(rows, UnorderedEquals(expected));
                }
                REQUIRE(insertedId == 4);
                user.name = "Sia";
                insertedId = storage.execute(statement);
                {
                    auto rows = storage.get_all<User>();
                    std::vector<User> expected;
                    expected.push_back(User{1, "Team BS"});
                    expected.push_back(User{2, "Shy'm"});
                    expected.push_back(User{3, "Maître Gims"});
                    expected.push_back(User{4, "Stromae"});
                    expected.push_back(User{5, "Stromae"});
                    REQUIRE_THAT(rows, UnorderedEquals(expected));
                }
                REQUIRE(insertedId == 5);
                
                get<0>(statement).name = "Sia";
                insertedId = storage.execute(statement);
                {
                    auto rows = storage.get_all<User>();
                    std::vector<User> expected;
                    expected.push_back(User{1, "Team BS"});
                    expected.push_back(User{2, "Shy'm"});
                    expected.push_back(User{3, "Maître Gims"});
                    expected.push_back(User{4, "Stromae"});
                    expected.push_back(User{5, "Stromae"});
                    expected.push_back(User{6, "Sia"});
                    REQUIRE_THAT(rows, UnorderedEquals(expected));
                }
            }
        }
    }
    SECTION("replace") {
        std::vector<User> expected;
        User user;
        SECTION("by ref existing") {
            user = {1, "Stromae"};
            expected.push_back(User{1, "Stromae"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            auto statement = storage.prepare(replace(user));
            storage.execute(statement);
            
            std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
            REQUIRE(user == get<0>(statement));
            REQUIRE(&user == &get<0>(statement));
        }
        SECTION("by ref new") {
            user = {4, "Stromae"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(user);
            auto statement = storage.prepare(replace(user));
            storage.execute(statement);
            auto rows = storage.get_all<User>();
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            
            user = {5, "LP"};
            expected.push_back(user);
            storage.execute(statement);
            
            REQUIRE(user == get<0>(statement));
            REQUIRE(&user == &get<0>(statement));
        }
        SECTION("by val existing") {
            SECTION("straight assign") {
                user = {1, "Stromae"};
            }
            expected.push_back(User{1, "Stromae"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            auto statement = storage.prepare(replace<by_val<User>>(user));
            REQUIRE(&user != &get<0>(statement));
            SECTION("assign with get") {
                get<0>(statement) = {1, "Stromae"};
            }
            storage.execute(statement);
        }
        SECTION("by val new") {
            user = {4, "Stromae"};
            expected.push_back(User{1, "Team BS"});
            expected.push_back(User{2, "Shy'm"});
            expected.push_back(User{3, "Maître Gims"});
            expected.push_back(user);
            auto statement = storage.prepare(replace<by_val<User>>(user));
            REQUIRE(&user != &get<0>(statement));
            storage.execute(statement);
            auto rows = storage.get_all<User>();
            REQUIRE_THAT(rows, UnorderedEquals(expected));
            
            user = {5, "LP"};
            storage.execute(statement);
        }
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
            REQUIRE(get<0>(statement) == users.begin());
            REQUIRE(get<1>(statement) == users.end());
            storage.execute(statement);
            expected.push_back(user);
        }
        SECTION("two") {
            User user1{4, "The Weeknd"};
            User user2{5, "Eva"};
            users.push_back(user1);
            users.push_back(user2);
            
            auto statement = storage.prepare(insert_range(users.begin(), users.end()));
            REQUIRE(get<0>(statement) == users.begin());
            REQUIRE(get<1>(statement) == users.end());
            storage.execute(statement);
            expected.push_back(user1);
            expected.push_back(user2);
            
            decltype(users) otherUsers;
            otherUsers.push_back(User{6, "DJ Alban"});
            otherUsers.push_back(User{7, "Flo Rida"});
            for(auto &user : otherUsers) {
                expected.push_back(user);
            }
            get<0>(statement) = otherUsers.begin();
            get<1>(statement) = otherUsers.end();
            storage.execute(statement);
            
            std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
            std::ignore = get<1>(static_cast<const decltype(statement) &>(statement));
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
    SECTION("insert explicit") {
        SECTION("user two columns") {
            User user{5, "Eminem"};
            SECTION("by ref") {
                auto statement = storage.prepare(insert(user, columns(&User::id, &User::name)));
                std::ignore = get<0>(static_cast<const decltype(statement) &>(statement));
                {
                    auto insertedId = storage.execute(statement);
                    REQUIRE(insertedId == user.id);
                    REQUIRE(storage.count<User>(where(is_equal(&User::name, "Eminem"))) == 1);
                }
                {
                    user.id = 6;
                    user.name = "Nate Dogg";
                    auto insertedId = storage.execute(statement);
                    REQUIRE(insertedId == 6);
                    REQUIRE(storage.count<User>(where(is_equal(&User::name, "Nate Dogg"))) == 1);
                    REQUIRE(&user == &get<0>(statement));
                }
            }
            SECTION("by val") {
                auto statement = storage.prepare(insert<by_val<User>>(user, columns(&User::id, &User::name)));
                {
                    auto insertedId = storage.execute(statement);
                    REQUIRE(insertedId == user.id);
                    REQUIRE(storage.count<User>(where(is_equal(&User::name, "Eminem"))) == 1);
                }
                {
                    user.id = 6;
                    user.name = "Nate Dogg";
                    try{
                        storage.execute(statement);
                        REQUIRE(false);
                    }catch(const std::system_error &e) {
                        REQUIRE(storage.count<User>(where(is_equal(&User::name, "Nate Dogg"))) == 0);
                    }catch(...){
                        REQUIRE(false);
                    }
                    
                    get<0>(statement) = user;
                    auto insertedId = storage.execute(statement);
                    REQUIRE(insertedId == user.id);
                    REQUIRE(storage.count<User>(where(is_equal(&User::name, "Nate Dogg"))) == 1);
                    REQUIRE(&user != &get<0>(statement));
                }
            }
        }
        SECTION("user id column only") {
            User user{4, "Eminem"};
            auto statement = storage.prepare(insert(user, columns(&User::name)));
            auto insertedId = storage.execute(statement);
            REQUIRE(insertedId == user.id);
        }
        SECTION("visit") {
            {
                Visit visit{1, 1, 100000};
                auto statement = storage.prepare(insert(visit, columns(&Visit::id, &Visit::userId, &Visit::time)));
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == visit.id);
            }
            {
                Visit visit{2, 1, defaultVisitTime + 1};    //  time must differ
                auto statement = storage.prepare(insert(visit, columns(&Visit::id, &Visit::userId)));
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == visit.id);
                auto insertedVisit = storage.get<Visit>(insertedId);
                REQUIRE(insertedVisit.id == visit.id);
                REQUIRE(insertedVisit.userId == visit.userId);
                REQUIRE(insertedVisit.time == defaultVisitTime);
            }
            {
                Visit visit{-1, 2, defaultVisitTime + 2};
                auto statement = storage.prepare(insert(visit, columns(&Visit::userId)));
                auto insertedId = storage.execute(statement);
                REQUIRE(insertedId == 3);
                auto insertedVisit = storage.get<Visit>(insertedId);
                REQUIRE(insertedVisit.id == 3);
                REQUIRE(insertedVisit.userId == visit.userId);
                REQUIRE(insertedVisit.time == defaultVisitTime);
            }
        }
    }
}
