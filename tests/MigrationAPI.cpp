#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

namespace version1 {

    using namespace sqlite_orm;

    struct User {
        int id = 0;
        std::string name;
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
    using migrator = std::function<void(const DbConnection&)>;

    std::map<std::pair<size_t, size_t>, migrator> migrations;  // <version pair, lambda>

    void register_migration(size_t from, size_t to, migrator m) {
        auto p = std::make_pair(from, to);
        migrations[p] = m;
    }

    void migrate_to(int target_version, const DbConnection& con) {
        int current{0};
        {
            auto storage = make_storage(con);  // just enough to get the version!
            current = storage.pragma.user_version();
        }
        auto p = std::make_pair(current, target_version);
        if(migrations.find(p) != migrations.end()) {
            auto lambda = migrations[p];
            lambda(con);
            return;
        }

        while(current < target_version) {
            auto p = std::make_pair(current, current + 1);
            if(migrations.find(p) != migrations.end()) {
                auto lambda = migrations[p];
                lambda(con);
                ++current;
            } else
                break;
        }
    }
}

TEST_CASE("DbConnection") {
    // insert using the original schema!
    struct OldUser {
        int id = 0;
        std::string name;
    };
    DbConnection con{"migration_api.sqlite"};
    REQUIRE(con.connection.use_count() == 1);

    auto oldStorage = make_storage(
        con,
        make_table("users", make_column("id", &OldUser::id, primary_key()), make_column("name", &OldUser::name)));
    // This checks whether another connection is opened - which shouldn't because all make_storages() use the DbConnection sqlite3*
    sqlite3* pDb = nullptr;
    oldStorage.on_open = [&pDb](sqlite3* p) {
        pDb =
            p;  // must never be called because on_open_internal is only called once in DbConnection's constructor (since every further call uses an open shared connection)
    };

    oldStorage.sync_schema();
    oldStorage.remove_all<OldUser>();
    oldStorage.pragma.user_version(1);
    oldStorage.replace(OldUser{1, "juan dent"});
    oldStorage.replace(OldUser{2, "klaus"});
    oldStorage.replace(OldUser{3, "eugene"});
    REQUIRE(oldStorage.count<OldUser>() == 3);
    REQUIRE(pDb == nullptr);

    SECTION("register_migrations") {  // TODO: These must be moved as methods of storage_base later!!
        register_migration(1, 2, [](const DbConnection& con) {
            std::map<int, std::string> emails = {
                {1, "fnc12@me.com"},
                {2, "megasuperhero@gmail.com"},
                {3, "thor@hotmail.com"},
            };
            auto oldStorage = version1::getStorage(con);
            auto allOldUsers = oldStorage.get_all<version1::User>();
            auto nextStorage = version2::getStorage(con);
            nextStorage.sync_schema();  // old users are dropped here
            nextStorage.pragma.user_version(2);
            for(auto& oldUser: allOldUsers) {
                version2::User newUser{oldUser.id, move(oldUser.name), emails[oldUser.id]};
                nextStorage.insert(newUser);
            }
        });
        register_migration(2, 3, [](const DbConnection& con) {
            auto oldStorage = version2::getStorage(con);
            auto allOldUsers = oldStorage.get_all<version2::User>();
            auto nextStorage = version3::getStorage(con);

            nextStorage.sync_schema();  // old users are dropped here
            nextStorage.pragma.user_version(3);

            for(auto& oldUser: allOldUsers) {
                auto& oldUserName = oldUser.name;
                auto spaceIndex = oldUserName.find(' ');
                version3::User newUser{oldUser.id};
                if(spaceIndex != oldUserName.npos) {  // space is found
                    auto firstName = oldUserName.substr(0, spaceIndex);
                    auto lastName = oldUserName.substr(spaceIndex, oldUserName.size() - spaceIndex);
                    newUser.first_name = move(firstName);
                    newUser.last_name = move(lastName);
                } else {
                    newUser.first_name = move(oldUserName);
                    newUser.last_name = {};
                }
                nextStorage.insert(newUser);
            }
        });
        migrate_to(3, con);
    }
}
