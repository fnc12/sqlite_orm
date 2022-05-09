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

    auto oldStorage = make_storage(
        con,
        make_table("users", make_column("id", &OldUser::id, primary_key()), make_column("name", &OldUser::name)));
    auto retain_count = oldStorage.get_connection_holder()->retain_count();
    REQUIRE(retain_count == 1);
    REQUIRE(con.connection.use_count() == 2);

    // This checks whether another connection is opened - which shouldn't because all make_storages() use the DbConnection sqlite3*
    sqlite3* pDb = nullptr;
    oldStorage.on_open = [&pDb](sqlite3* p) {
        pDb =
            p;  // must never be called because on_open_internal is only called once in DbConnection's constructor (since every further call uses an open shared connection)
    };
    oldStorage.drop_table(oldStorage.tablename<OldUser>());
    oldStorage.sync_schema();
    oldStorage.pragma.user_version(1);
    oldStorage.replace(OldUser{1, "juan dent"});
    oldStorage.replace(OldUser{2, "klaus"});
    oldStorage.replace(OldUser{3, "eugene"});
    retain_count = oldStorage.get_connection_holder()->retain_count();
    REQUIRE(retain_count == 1);
    REQUIRE(con.connection.use_count() == 2);
    REQUIRE(pDb == nullptr);

    SECTION("register_migrations") {
        oldStorage.register_migration(1, 2, [](const DbConnection& con) {
            internal::connection_ref{*con.connection};
            std::map<int, std::string> emails = {
                {1, "fnc12@me.com"},
                {2, "megasuperhero@gmail.com"},
                {3, "thor@hotmail.com"},
            };
            auto oldStorage = version1::getStorage(con);
            auto retain_count = oldStorage.get_connection_holder()->retain_count();
            REQUIRE(retain_count == 1);
            auto allOldUsers = oldStorage.get_all<version1::User>();
            auto nextStorage = version2::getStorage(con);
            retain_count = nextStorage.get_connection_holder()->retain_count();
            nextStorage.sync_schema();  // old users are dropped here
            nextStorage.pragma.user_version(2);
            retain_count = nextStorage.get_connection_holder()->retain_count();
            REQUIRE(retain_count == 1);
            for(auto& oldUser: allOldUsers) {
                version2::User newUser{oldUser.id, move(oldUser.name), emails[oldUser.id]};
                nextStorage.insert(newUser);
            }
            auto count = con.connection.use_count();
        });
        oldStorage.register_migration(2, 3, [](const DbConnection& con) {
            internal::connection_ref{*con.connection};
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

        oldStorage.migrate_to(3, con);
        REQUIRE(1 == 1);
    }
}
