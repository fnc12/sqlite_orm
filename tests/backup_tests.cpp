#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstdio>  //  remove

using namespace sqlite_orm;

namespace {
    struct User {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User() = default;
        User(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };

    bool operator==(const User& lhs, const User& rhs) {
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }

    struct MarvelHero {
        int id;
        std::string name;
        std::string abilities;
    };

    auto initStorageMarvel(const std::string& path) {
        auto storage = make_storage(path,
                                    make_table("marvel",
                                               make_column("id", &MarvelHero::id, primary_key()),
                                               make_column("name", &MarvelHero::name),
                                               make_column("abilities", &MarvelHero::abilities)));
        return storage;
    }
}

TEST_CASE("backup") {
    using Catch::Matchers::UnorderedEquals;

    const std::string usersTableName = "users";
    auto makeStorage = [&usersTableName](const std::string& filename) {
        return make_storage(
            filename,
            make_table(usersTableName, make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    };
    static int filenameSuffix = 0;  //  to make an unique filename for every test
    const std::string backupFilename = "backup" + std::to_string(filenameSuffix++) + ".sqlite";
    SECTION("to") {
        ::remove(backupFilename.c_str());
        auto storage2 = makeStorage(backupFilename);
        auto storage = makeStorage("");
        storage.sync_schema();
        storage.replace(User{1, "Sharon"});
        storage.replace(User{2, "Maitre"});
        storage.replace(User{3, "Rita"});
        REQUIRE(!storage2.table_exists(usersTableName));
        SECTION("filename") {
            storage.backup_to(backupFilename);
        }
        SECTION("storage") {
            storage.backup_to(storage2);
        }
        SECTION("filename step -1") {
            auto backup = storage.make_backup_to(backupFilename);
            backup.step(-1);
        }
        SECTION("filename step 1") {
            auto backup = storage.make_backup_to(backupFilename);
            do {
                backup.step(1);
            } while(backup.remaining() > 0);
        }
        SECTION("storage step -1") {
            auto backup = storage.make_backup_to(storage2);
            backup.step(-1);
        }
        SECTION("storage step 1") {
            auto backup = storage.make_backup_to(storage2);
            do {
                backup.step(1);
            } while(backup.remaining() > 0);
        }
        REQUIRE(storage2.table_exists(usersTableName));
        auto rowsFromBackup = storage2.get_all<User>();
        auto expectedRows = storage.get_all<User>();
        REQUIRE_THAT(rowsFromBackup, UnorderedEquals(expectedRows));
    }
    SECTION("from") {
        ::remove(backupFilename.c_str());
        auto storage = makeStorage(backupFilename);
        storage.sync_schema();
        storage.replace(User{1, "Sharon"});
        storage.replace(User{2, "Maitre"});
        storage.replace(User{3, "Rita"});
        auto storage2 = makeStorage("");
        SECTION("filename") {
            storage2.backup_from(backupFilename);
        }
        SECTION("filename step -1") {
            auto backup = storage2.make_backup_from(backupFilename);
            backup.step(-1);
        }
        SECTION("filename step 1") {
            auto backup = storage2.make_backup_from(backupFilename);
            do {
                backup.step(1);
            } while(backup.remaining() > 0);
        }
        SECTION("storage") {
            storage2.backup_from(storage);
        }
        SECTION("storage step -1") {
            auto backup = storage2.make_backup_from(storage);
            backup.step(-1);
        }
        SECTION("storage step -1") {
            auto backup = storage2.make_backup_from(storage);
            do {
                backup.step(1);
            } while(backup.remaining() > 0);
        }
        REQUIRE(storage2.table_exists(usersTableName));
        auto rowsFromBackup = storage2.get_all<User>();
        REQUIRE_THAT(rowsFromBackup, UnorderedEquals(storage.get_all<User>()));
    }
}

TEST_CASE("Backup crash") {
    using MarvelStorage = decltype(initStorageMarvel(""));

    // --- Create a shared pointer to the MarvelStorage
    std::string fp("iteration.sqlite");
    std::shared_ptr<MarvelStorage> db = std::make_shared<MarvelStorage>(initStorageMarvel(fp));
    auto storage{*db};

    storage.sync_schema();
    storage.remove_all<MarvelHero>();

    // --- Insert values
    storage.insert(MarvelHero{-1, "Tony Stark", "Iron man, playboy, billionaire, philanthropist"});
    storage.insert(MarvelHero{-1, "Thor", "Storm god"});
    storage.insert(MarvelHero{-1, "Vision", "Min Stone"});
    storage.insert(MarvelHero{-1, "Captain America", "Vibranium shield"});
    storage.insert(MarvelHero{-1, "Hulk", "Strength"});
    storage.insert(MarvelHero{-1, "Star Lord", "Humor"});
    storage.insert(MarvelHero{-1, "Peter Parker", "Spiderman"});
    storage.insert(MarvelHero{-1, "Clint Barton", "Hawkeye"});
    storage.insert(MarvelHero{-1, "Natasha Romanoff", "Black widow"});
    storage.insert(MarvelHero{-1, "Groot", "I am Groot!"});
    REQUIRE(storage.count<MarvelHero>() == 10);

    // --- Create backup file name and verify that the file does not exist
    std::string backupFilename{"backup.sqlite"};

    // --- Backup the current storage to the file
    auto backup = storage.make_backup_to(backupFilename);
    backup.step(-1);
}
