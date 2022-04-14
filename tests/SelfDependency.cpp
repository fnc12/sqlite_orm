#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <filesystem>
#include <iostream>
#include <type_traits>  //  std::is_same

using namespace sqlite_orm;





TEST_CASE("self dependency") {
    SECTION("with backup db") {
        struct User {
            int id = 0;
            std::string name;
            std::optional<int> manager;
        };
        struct Registration {
            int id;
            std::optional<int> userid;
        };

        auto create = [](std::string dbFileName) {
            auto storage = make_storage(dbFileName,
                make_table("users",
                    make_column("id", &User::id, primary_key(), autoincrement()),
                    make_column("name", &User::name),
                    make_column("manager", &User::manager),
                    foreign_key(&User::manager).references(&User::id)),
                make_table("reg",
                    make_column("id", &Registration::id, primary_key(), autoincrement()),
                    make_column("userid", &Registration::userid),
                    foreign_key(&Registration::userid).references(&User::id)));

            return storage;
        };



        auto storage = create("self_dependency.sqlite");

        auto ret = storage.sync_schema(true);
        for (auto r : ret) {
            std::cout << r.first << " " << r.second << std::endl;
        }
        namespace fs = std::filesystem;
        bool ok = fs::exists("self_dependency.sqlite");
        REQUIRE(ok == true);

        // empty db
        storage.remove_all<Registration>();
        storage.remove_all<User>();

        // define data
        User us1 {1, "Michael", std::nullopt};
        User us2 {2, "Juan", std::nullopt};

        // insert data
        storage.replace(us1);
        storage.replace(us2);

        auto user = storage.get<User>(1);
        user.manager = 2;
        storage.update(user);
        storage.remove_all<Registration>();
        Registration reg{ 1,1 };
        Registration reg2{ 2,2 };
        storage.replace(reg);
        storage.replace(reg2);
        ///////// DATA INSERTED ////////

        auto backup = create("self_dependency_backup.sqlite");
        backup.sync_schema(true);
        backup.remove_all<User>();

        SECTION("Cannot backup") {
            try {
                // make backup  // ASSUME User's table is HUGE cannot load it into memory!
                for (auto r : storage.iterate<User>()) {
                    backup.replace(r);
                }

                // drop table
                storage.drop_table(storage.tablename<User>());

                // recreate table
                ret = storage.sync_schema(true);

                // load data from backup
                for (auto r : backup.iterate<User>()) {
                    storage.replace(r);
                }

            }
            catch (std::exception& ex) {
                std::string s = ex.what();
                REQUIRE(s.starts_with("FOREIGN KEY"));        // CANNOT MAKE BACKUP, CANNOT LOAD FROM BACKUP!!
                std::ignore = s;
            }
        }
        SECTION("Turn off FKs") {
            backup.foreign_key(false);
            storage.foreign_key(false);
            try {
                // make backup  // ASSUME User's table is HUGE cannot load it into memory!
                for (auto r : storage.iterate<User>()) {
                    backup.replace(r);
                }

                // drop table
                storage.drop_table(storage.tablename<User>());

                // recreate table
                ret = storage.sync_schema(true);

                // load data from backup
                for (auto r : backup.iterate<User>()) {
                    storage.replace(r);
                }

            }
            catch (std::exception& ex) {
                std::string s = ex.what();
                std::ignore = s;
            }
            backup.foreign_key(true);
            storage.foreign_key(true);

        }
        SECTION("backup api") {
            auto back = create("tmp.sqlite");
            auto orig = storage.get_all<User>();
            storage.backup_to(back);
            auto vec = back.get_all<User>();

            try {
                storage.rename_table<User>("user_temp");
                storage.sync_schema(true);
                storage.backup_from("tmp.sqlite");
                auto orig = storage.get_all<User>();
                int i = 0;
            }
            catch(std::exception& ex) {
                std::string s = ex.what();
                std::ignore = s;
            }
        }
    }
}
