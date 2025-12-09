#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <algorithm>
#include "../catch_matchers.h"

#include "prepared_common.h"

#if SQLITE_VERSION_NUMBER >= 3006019
using namespace sqlite_orm;

TEST_CASE("Prepared insert range") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;
    const ErrorCodeExceptionMatcher emptyRangeExceptionMatcher(orm_error_code::empty_range);

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key().autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time, default_value(defaultVisitTime)),
                                           foreign_key(&Visit::userId).references(&User::id)),
                                make_table("users_and_visits",
                                           make_column("user_id", &UserAndVisit::userId),
                                           make_column("visit_id", &UserAndVisit::visitId),
                                           make_column("description", &UserAndVisit::description),
                                           primary_key(&UserAndVisit::userId, &UserAndVisit::visitId)));
    storage.sync_schema();

    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});

    storage.insert(UserAndVisit{2, 1, "Glad you came"});
    storage.insert(UserAndVisit{3, 1, "Shine on"});

    std::vector<User> users;
    std::vector<User> expected{{1, "Team BS"}, {2, "Shy'm"}, {3, "Maître Gims"}};

    SECTION("empty") {
        SECTION("strict") {
            REQUIRE_THROWS_MATCHES(storage.prepare(insert_range(users.begin(), users.end())),
                                   std::system_error,
                                   emptyRangeExceptionMatcher);
        }
        SECTION("container with pointers") {
            std::vector<std::unique_ptr<User>> usersPointers;
            REQUIRE_THROWS_MATCHES(
                storage.prepare(
                    insert_range(usersPointers.begin(), usersPointers.end(), &std::unique_ptr<User>::operator*)),
                std::system_error,
                emptyRangeExceptionMatcher);
        }
    }
    SECTION("one") {
        User user{4, "The Weeknd"};
        users.push_back(user);
        SECTION("strict container") {
            auto statement = storage.prepare(insert_range(users.begin(), users.end()));
            REQUIRE(get<0>(statement) == users.begin());
            REQUIRE(get<1>(statement) == users.end());
            storage.execute(statement);
        }
        SECTION("container of pointers") {
            std::vector<std::unique_ptr<User>> usersPointers;
            usersPointers.reserve(users.size());
            std::transform(users.begin(), users.end(), std::back_inserter(usersPointers), [](const User& user) {
                return std::make_unique<User>(user);
            });
            auto statement = storage.prepare(
                insert_range(usersPointers.begin(), usersPointers.end(), &std::unique_ptr<User>::operator*));
            REQUIRE(get<0>(statement) == usersPointers.begin());
            REQUIRE(get<1>(statement) == usersPointers.end());
            storage.execute(statement);
        }
        SECTION("container with references") {
            std::vector<std::reference_wrapper<User>> usersRefs{users.begin(), users.end()};
            auto statement = storage.prepare(insert_range<User>(usersRefs.begin(), usersRefs.end()));
            REQUIRE(get<0>(statement) == usersRefs.begin());
            REQUIRE(get<1>(statement) == usersRefs.end());
            storage.execute(statement);
        }
        expected.push_back(user);
    }
    SECTION("two") {
        User user1{4, "The Weeknd"};
        User user2{5, "Eva"};
        users.push_back(user1);
        users.push_back(user2);

        SECTION("strict") {
            auto statement = storage.prepare(insert_range(users.begin(), users.end()));
            REQUIRE(get<0>(statement) == users.begin());
            REQUIRE(get<1>(statement) == users.end());
            storage.execute(statement);
            expected.push_back(user1);
            expected.push_back(user2);

            decltype(users) otherUsers;
            otherUsers.push_back(User{6, "DJ Alban"});
            otherUsers.push_back(User{7, "Flo Rida"});
            for (auto& user: otherUsers) {
                expected.push_back(user);
            }
            get<0>(statement) = otherUsers.begin();
            get<1>(statement) = otherUsers.end();
            storage.execute(statement);

            std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
            std::ignore = get<1>(static_cast<const decltype(statement)&>(statement));
        }
        SECTION("container of pointers") {
            std::vector<std::unique_ptr<User>> usersPointers;
            std::transform(users.begin(), users.end(), std::back_inserter(usersPointers), [](const User& user) {
                return std::make_unique<User>(user);
            });
            auto statement = storage.prepare(
                insert_range(usersPointers.begin(), usersPointers.end(), &std::unique_ptr<User>::operator*));
            REQUIRE(get<0>(statement) == usersPointers.begin());
            REQUIRE(get<1>(statement) == usersPointers.end());
            storage.execute(statement);
            expected.push_back(user1);
            expected.push_back(user2);

            decltype(usersPointers) otherUsers;
            otherUsers.emplace_back(new User{6, "DJ Alban"});
            otherUsers.emplace_back(new User{7, "Flo Rida"});
            for (auto& user: otherUsers) {
                expected.push_back(*user);
            }
            get<0>(statement) = otherUsers.begin();
            get<1>(statement) = otherUsers.end();
            storage.execute(statement);

            std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
            std::ignore = get<1>(static_cast<const decltype(statement)&>(statement));
        }
    }
    auto rows = storage.get_all<User>();
    REQUIRE_THAT(rows, UnorderedEquals(expected));
}
#endif
