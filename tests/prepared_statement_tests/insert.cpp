#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

using namespace sqlite_orm;

TEST_CASE("Prepared insert") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    struct Artist {
        int id = 0;
        std::unique_ptr<std::string> name;

        Artist() = default;

        Artist(decltype(id) id_, decltype(name) name_) : id(id_), name(std::move(name_)) {}

        Artist(const Artist& other) :
            id(other.id), name(other.name ? std::make_unique<std::string>(*other.name) : nullptr) {}

        bool operator==(const Artist& other) const {
            return this->id == other.id && this->compareNames(this->name, other.name);
        }

        bool compareNames(const decltype(name)& lhs, const decltype(name)& rhs) const {
            if(lhs && rhs) {
                return *lhs == *rhs;
            } else if(!lhs && !rhs) {
                return true;
            } else {
                return false;
            }
        }
    };

    struct ArtistBackup : Artist {
        ArtistBackup() = default;
        ArtistBackup(Artist other) : Artist(std::move(other)) {}
    };

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(
        filename,
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
                   primary_key(&UserAndVisit::userId, &UserAndVisit::visitId)),
        make_table("artists", make_column("id", &Artist::id, primary_key()), make_column("name", &Artist::name)),
        make_table<ArtistBackup>("artists_backup",
                                 make_column("id", &Artist::id, primary_key()),
                                 make_column("name", &Artist::name)));
    storage.sync_schema();
    SECTION("raw insert") {
        SECTION("values") {
            std::vector<User> allUsers;
            decltype(allUsers) expected;
            SECTION("one user") {
                SECTION("statement") {
                    auto statement = storage.prepare(
                        insert(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, "Ellie"))));
                    storage.execute(statement);
                    REQUIRE(get<0>(statement) == 1);
                    REQUIRE(::strcmp(get<1>(statement), "Ellie") == 0);
                }
                SECTION("no statement") {
                    storage.insert(into<User>(), columns(&User::id, &User::name), values(std::make_tuple(1, "Ellie")));
                }

                allUsers = storage.get_all<User>();
                expected.push_back({1, "Ellie"});
            }
            SECTION("two users") {
                SECTION("statement") {
                    auto statement =
                        storage.prepare(insert(into<User>(),
                                               columns(&User::id, &User::name),
                                               values(std::make_tuple(1, "Ellie"), std::make_tuple(5, "Calvin"))));
                    storage.execute(statement);
                    REQUIRE(get<0>(statement) == 1);
                    REQUIRE(::strcmp(get<1>(statement), "Ellie") == 0);
                    REQUIRE(get<2>(statement) == 5);
                    REQUIRE(::strcmp(get<3>(statement), "Calvin") == 0);
                }
                SECTION("no statement") {
                    storage.insert(into<User>(),
                                   columns(&User::id, &User::name),
                                   values(std::make_tuple(1, "Ellie"), std::make_tuple(5, "Calvin")));
                }

                allUsers = storage.get_all<User>();
                expected.push_back({1, "Ellie"});
                expected.push_back({5, "Calvin"});
            }
            REQUIRE_THAT(allUsers, UnorderedEquals(expected));
        }
        SECTION("default values") {
            SECTION("statement") {
                auto statement = storage.prepare(insert(into<Artist>(), default_values()));
                storage.execute(statement);
            }
            SECTION("no statement") {
                storage.insert(into<Artist>(), default_values());
            }

            auto allArtists = storage.get_all<Artist>();
            decltype(allArtists) expected;
            expected.push_back({1, nullptr});
            REQUIRE(allArtists == expected);
        }
        SECTION("select") {
            Artist artist1{1, std::make_unique<std::string>("Deepend")};
            Artist artist4{4, std::make_unique<std::string>("Robin Schulz")};
            storage.replace(artist1);
            storage.replace(artist4);
            REQUIRE(storage.count<ArtistBackup>() == 0);
            SECTION("statement") {
                auto statement =
                    storage.prepare(insert(into<ArtistBackup>(), select(columns(&Artist::id, &Artist::name))));
                storage.execute(statement);
            }
            SECTION("no statement") {
                storage.insert(into<ArtistBackup>(), select(columns(&Artist::id, &Artist::name)));
            }

            auto allArtistBackups = storage.get_all<ArtistBackup>();
            decltype(allArtistBackups) expected;
            expected.push_back(artist1);
            expected.push_back(artist4);
            REQUIRE_THAT(allArtistBackups, UnorderedEquals(expected));
        }
    }
    SECTION("insert object") {
        storage.replace(User{1, "Team BS"});
        storage.replace(User{2, "Shy'm"});
        storage.replace(User{3, "Maître Gims"});

        storage.replace(UserAndVisit{2, 1, "Glad you came"});
        storage.replace(UserAndVisit{3, 1, "Shine on"});

        User user{0, "Stromae"};
        SECTION("by ref") {
            auto statement = storage.prepare(insert(std::ref(user)));
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
                std::ignore = get<0>(static_cast<const decltype(statement)&>(statement));
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
}
