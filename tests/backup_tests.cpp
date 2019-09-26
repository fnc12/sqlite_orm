#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <cstdio>   //  remove

using namespace sqlite_orm;

namespace BackupTests {
    struct User {
        int id = 0;
        std::string name;
    };
    
    bool operator==(const User &lhs, const User &rhs) {
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }
}

TEST_CASE("backup") {
    using namespace BackupTests;
    using Catch::Matchers::UnorderedEquals;
    const std::string usersTableName = "users";
    auto makeStorage = [&usersTableName](const std::string &filename){
        return make_storage(filename,
                            make_table(usersTableName,
                                       make_column("id", &User::id, primary_key()),
                                       make_column("name", &User::name)));
    };
    const std::string backupFilename = "backup.sqlite";
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
            do{
                backup.step(1);
            }while(backup.remaining() > 0);
        }
        SECTION("storage step -1") {
            auto backup = storage.make_backup_to(storage2);
            backup.step(-1);
        }
        SECTION("storage step 1") {
            auto backup = storage.make_backup_to(storage2);
            do{
                backup.step(1);
            }while(backup.remaining() > 0);
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
            do{
                backup.step(1);
            }while(backup.remaining() > 0);
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
            do{
                backup.step(1);
            }while(backup.remaining() > 0);
        }
        REQUIRE(storage2.table_exists(usersTableName));
        auto rowsFromBackup = storage2.get_all<User>();
        REQUIRE_THAT(rowsFromBackup, UnorderedEquals(storage.get_all<User>()));
    }
}
