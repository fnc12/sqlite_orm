#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace version1 {

    using namespace sqlite_orm;

    struct User {
        int id = 0;
        std::string name;
#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User(int id, std::string name) : id{id}, name(name) {}
        User() = default;
#endif
    };

    auto getStorage(const DbConnection& con) {
        auto storage = make_storage(
            con,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
        return storage;
    }
}

namespace version2 {
    using namespace sqlite_orm;

    struct User {
        int id = 0;
        std::string name;
        std::string email;
#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User(int id, std::string name, std::string email) : id{id}, name(name), email{email} {}
        User() = default;
#endif
    };

    auto getStorage(const DbConnection& con) {
        auto storage = make_storage(con,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("name", &User::name),
                                               make_column("email", &User::email)));
        return storage;
    }
}

namespace version3 {
    using namespace sqlite_orm;

    struct User {
        int id = 0;
        std::string first_name;
        std::string last_name;
        std::string email;
#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        User(int id, std::string fn, std::string ln, std::string email) :
            id{id}, first_name(fn), last_name(ln), email{email} {}
        User(int id) : id{id} {}
#endif
    };

    auto getStorage(const DbConnection& con) {
        auto storage = make_storage(con,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("first_name", &User::first_name),
                                               make_column("last_name", &User::last_name),
                                               make_column("email", &User::email)));
        return storage;
    }
}

namespace {
    struct OldUser {
        int id = 0;
        std::string name;
#ifndef SQLITE_ORM_AGGREGATE_NSDMI_SUPPORTED
        OldUser(int i, std::string name) : id{id}, name{name} {}
#endif
    };
}

TEST_CASE("DbConnectionAPI") {
    // insert using the original schema!
    DbConnection con{"migration_api.sqlite"};
    REQUIRE(con.connection.use_count() == 1);

    auto storage = make_storage(
        con,
        make_table("users", make_column("id", &OldUser::id, primary_key()), make_column("name", &OldUser::name)));
    auto retain_count = storage.retain_count();
    REQUIRE(retain_count == 1);
    REQUIRE(con.connection.use_count() == 2);

    // This checks whether another connection is opened - which shouldn't because all make_storages() use the DbConnection sqlite3*
    int open_counter = 0;
    storage.on_open = [&open_counter](sqlite3* p) {
        open_counter++;
    // must never be called because on_open_internal is only called once in DbConnection's constructor (since every further call uses an open shared connection)
    };
    storage.drop_table(storage.tablename<OldUser>());
    storage.sync_schema();
    storage.pragma.user_version(1);
    storage.replace(OldUser{1, "juan dent"});
    storage.replace(OldUser{2, "klaus"});
    storage.replace(OldUser{3, "eugene"});
    retain_count = storage.retain_count();
    REQUIRE(retain_count == 1);
    REQUIRE(con.connection.use_count() == 2);
    REQUIRE(open_counter == 0);

    SECTION("register_migrations") {
        storage.register_migration(1, 2, [](const DbConnection& con) {
            std::map<int, std::string> emails = {
                {1, "fnc12@me.com"},
                {2, "megasuperhero@gmail.com"},
                {3, "thor@hotmail.com"},
            };
            auto oldStorage = version1::getStorage(con);
            auto retain_count = oldStorage.retain_count();
            REQUIRE(retain_count == 1);
            auto allOldUsers = oldStorage.get_all<version1::User>();
            auto nextStorage = version2::getStorage(con);
            retain_count = nextStorage.retain_count();
            nextStorage.sync_schema();  // old users are dropped here
            // nextStorage.pragma.user_version(2);
            retain_count = nextStorage.retain_count();
            REQUIRE(retain_count == 1);
            for(auto& oldUser: allOldUsers) {
                version2::User newUser{oldUser.id, move(oldUser.name), emails[oldUser.id]};
                nextStorage.insert(newUser);
            }
        });
        storage.register_migration(2, 3, [](const DbConnection& con) {
            auto oldStorage = version2::getStorage(con);
            auto allOldUsers = oldStorage.get_all<version2::User>();
            int ver = oldStorage.pragma.user_version();

            auto nextStorage = version3::getStorage(con);

            nextStorage.sync_schema();  // old users are dropped here
            ver = nextStorage.pragma.user_version();

            // nextStorage.pragma.user_version(3);  // ?????

            for(auto& oldUser: allOldUsers) {
                auto& oldUserName = oldUser.name;
                auto spaceIndex = oldUserName.find(' ');
                version3::User newUser{oldUser.id};
                if(spaceIndex != oldUserName.npos) {  // space is found
                    auto firstName = oldUserName.substr(0, spaceIndex++);
                    auto lastName = oldUserName.substr(spaceIndex, oldUserName.size() - spaceIndex);
                    newUser.first_name = move(firstName);
                    newUser.last_name = move(lastName);
                } else {
                    newUser.first_name = move(oldUserName);
                    newUser.last_name = {};
                }
                newUser.email = oldUser.email;
                nextStorage.insert(newUser);
            }
        });

        storage.migrate_to(3, con);
    }
}
