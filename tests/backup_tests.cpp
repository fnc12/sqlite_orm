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
        auto storage = makeStorage("");
        storage.sync_schema();
        storage.replace(User{1, "Sharon"});
        storage.replace(User{2, "Maitre"});
        storage.replace(User{3, "Rita"});
        auto storage2 = makeStorage(backupFilename);
        SECTION("filename") {
            storage.backup_to(backupFilename);
        }
        SECTION("storage") {
            ::remove(backupFilename.c_str());
            storage.backup_to(storage2);
        }
        REQUIRE(storage2.table_exists(usersTableName));
        auto rowsFromBackup = storage2.get_all<User>();
        REQUIRE_THAT(rowsFromBackup, UnorderedEquals(storage.get_all<User>()));
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
        SECTION("storage") {
            storage2.backup_from(storage);
        }
        REQUIRE(storage2.table_exists(usersTableName));
        auto rowsFromBackup = storage2.get_all<User>();
        REQUIRE_THAT(rowsFromBackup, UnorderedEquals(storage.get_all<User>()));
    }
}
