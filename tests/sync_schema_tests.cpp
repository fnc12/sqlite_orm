#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

/**
 *  this is the deal: assume we have a `users` table with schema
 *  `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL, category_id INTEGER, surname TEXT)`.
 *  We create a storage and insert several objects. Next we simulate schema changing (app update for example): create
 *  another storage with a new schema partial of the previous one: `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY,
 * name TEXT NOT NULL)`. Next we call `sync_schema(true)` and assert that all users are saved. This test tests whether
 * REMOVE COLUMN imitation works well.
 */
TEST_CASE("Sync schema") {

    //  this is an old version of user..
    struct UserBefore {
        int id;
        std::string name;
        std::unique_ptr<int> categoryId;
        std::unique_ptr<std::string> surname;
    };

    //  this is a new version of user
    struct UserAfter {
        int id;
        std::string name;
    };

    //  create an old storage
    auto filename = "sync_schema_text.sqlite";
    auto storage = make_storage(filename,
                                make_table("users",
                                           make_column("id", &UserBefore::id, primary_key()),
                                           make_column("name", &UserBefore::name),
                                           make_column("category_id", &UserBefore::categoryId),
                                           make_column("surname", &UserBefore::surname)));

    //  sync in case if it is first launch
    auto syncSchemaSimulationRes = storage.sync_schema_simulate();
    auto syncSchemaRes = storage.sync_schema();

    REQUIRE(syncSchemaRes == syncSchemaSimulationRes);

    //  remove old users in case the test was launched before
    storage.remove_all<UserBefore>();

    //  create c++ objects to insert into table
    std::vector<UserBefore> usersToInsert;
    usersToInsert.push_back({-1, "Michael", nullptr, std::make_unique<std::string>("Scofield")});
    usersToInsert.push_back({-1, "Lincoln", std::make_unique<int>(4), std::make_unique<std::string>("Burrows")});
    usersToInsert.push_back({-1, "Sucre", nullptr, nullptr});
    usersToInsert.push_back({-1, "Sara", std::make_unique<int>(996), std::make_unique<std::string>("Tancredi")});
    usersToInsert.push_back({-1, "John", std::make_unique<int>(100500), std::make_unique<std::string>("Abruzzi")});
    usersToInsert.push_back({-1, "Brad", std::make_unique<int>(65), nullptr});
    usersToInsert.push_back({-1, "Paul", std::make_unique<int>(65), nullptr});

    for(auto& user: usersToInsert) {
        auto insertedId = storage.insert(user);
        user.id = insertedId;
    }

    //  assert count first cause we shall be asserting row by row next
    REQUIRE(static_cast<size_t>(storage.count<UserBefore>()) == usersToInsert.size());

    //  now we create a new storage with a partial schema
    auto newStorage = make_storage(
        filename,
        make_table("users", make_column("id", &UserAfter::id, primary_key()), make_column("name", &UserAfter::name)));

    syncSchemaSimulationRes = newStorage.sync_schema_simulate(true);

    //  now call `sync_schema` with argument `preserve` as `true`. It will retain the data in case `sqlite_orm` needs to
    //  remove a column
    syncSchemaRes = newStorage.sync_schema(true);
    REQUIRE(syncSchemaRes.size() == 1);
    REQUIRE(syncSchemaRes.begin()->second == sync_schema_result::old_columns_removed);
    REQUIRE(syncSchemaSimulationRes == syncSchemaRes);

    //  get all users after syncing the schema
    auto usersFromDb = newStorage.get_all<UserAfter>(order_by(&UserAfter::id));

    REQUIRE(usersFromDb.size() == usersToInsert.size());

    for(size_t i = 0; i < usersFromDb.size(); ++i) {
        auto& userFromDb = usersFromDb[i];
        auto& oldUser = usersToInsert[i];
        REQUIRE(userFromDb.id == oldUser.id);
        REQUIRE(userFromDb.name == oldUser.name);
    }

    auto usersCountBefore = newStorage.count<UserAfter>();

    syncSchemaSimulationRes = newStorage.sync_schema_simulate();
    syncSchemaRes = newStorage.sync_schema();
    REQUIRE(syncSchemaRes == syncSchemaSimulationRes);

    auto usersCountAfter = newStorage.count<UserAfter>();
    REQUIRE(usersCountBefore == usersCountAfter);

    //  test select..
    auto ids = newStorage.select(&UserAfter::id);
    auto users = newStorage.get_all<UserAfter>();
    decltype(ids) idsFromGetAll;
    idsFromGetAll.reserve(users.size());
    std::transform(users.begin(), users.end(), std::back_inserter(idsFromGetAll), [=](auto& user) {
        return user.id;
    });
    REQUIRE(std::equal(ids.begin(), ids.end(), idsFromGetAll.begin(), idsFromGetAll.end()));
}

TEST_CASE("issue521") {
    auto storagePath = "issue521.sqlite";

    struct MockDatabasePoco {
        int id{-1};
        std::string name{""};
        uint32_t alpha{0};
        float beta{0.0};
    };
    std::vector<MockDatabasePoco> pocosToInsert;

    ::remove(storagePath);
    {
        // --- Create the initial database
        auto storage = sqlite_orm::make_storage(
            storagePath,
            sqlite_orm::make_table("pocos",
                                   sqlite_orm::make_column("id", &MockDatabasePoco::id, sqlite_orm::primary_key()),
                                   sqlite_orm::make_column("name", &MockDatabasePoco::name)));

        // --- We simulate the synchronization first, then do it for real and compare
        auto simulated = storage.sync_schema_simulate(true);
        auto ssr = storage.sync_schema(true);
        REQUIRE(ssr == simulated);
        REQUIRE(ssr.at("pocos") == sqlite_orm::sync_schema_result::new_table_created);

        // --- Insert two rows
        pocosToInsert.clear();
        pocosToInsert.push_back({-1, "Michael", 10, 10.10});
        pocosToInsert.push_back({-1, "Joyce", 20, 20.20});

        for(auto& poco: pocosToInsert) {
            auto insertedId = storage.insert(poco);
            poco.id = insertedId;
        }

        // --- Retrieve the pocos and verify
        REQUIRE(static_cast<size_t>(storage.count<MockDatabasePoco>()) == pocosToInsert.size());

        using namespace sqlite_orm;
        auto pocosFromDb = storage.get_all<MockDatabasePoco>(order_by(&MockDatabasePoco::id));
        for(size_t i = 0; i < pocosFromDb.size(); ++i) {
            auto& pocoFromDb = pocosFromDb[i];
            auto& oldPoco = pocosToInsert[i];

            REQUIRE(pocoFromDb.id == oldPoco.id);
            REQUIRE(pocoFromDb.name == oldPoco.name);
        }
    }
    {
        // --- Read the database and create the storage
        auto storage = sqlite_orm::make_storage(
            storagePath,
            sqlite_orm::make_table("pocos",
                                   sqlite_orm::make_column("id", &MockDatabasePoco::id, sqlite_orm::primary_key()),
                                   sqlite_orm::make_column("name", &MockDatabasePoco::name)));
        // --- We simulate the synchronization first, then do it for real and compare
        auto simulated = storage.sync_schema_simulate(true);
        auto ssr = storage.sync_schema(true);
        REQUIRE(ssr == simulated);
        REQUIRE(ssr["pocos"] == sqlite_orm::sync_schema_result::already_in_sync);

        REQUIRE(static_cast<size_t>(storage.count<MockDatabasePoco>()) == pocosToInsert.size());

        auto pocosFromDb = storage.get_all<MockDatabasePoco>(order_by(&MockDatabasePoco::id));
        for(size_t i = 0; i < pocosFromDb.size(); ++i) {
            auto& pocoFromDb = pocosFromDb[i];
            auto& oldPoco = pocosToInsert[i];
            REQUIRE(pocoFromDb.id == oldPoco.id);
            REQUIRE(pocoFromDb.name == oldPoco.name);
        }
    }
    // --- Add a new column
    {
        // --- Read the database and create the storage
        auto storage = sqlite_orm::make_storage(
            storagePath,
            sqlite_orm::make_table(
                "pocos",
                sqlite_orm::make_column("id", &MockDatabasePoco::id, sqlite_orm::primary_key()),
                sqlite_orm::make_column("name", &MockDatabasePoco::name),
                sqlite_orm::make_column("alpha", &MockDatabasePoco::alpha, sqlite_orm::default_value(1))));
        // --- We simulate the synchronization first, then do it for real and compare
        auto simulated = storage.sync_schema_simulate(true);
        auto ssr = storage.sync_schema(true);
        REQUIRE(ssr == simulated);
        REQUIRE(ssr["pocos"] == sqlite_orm::sync_schema_result::new_columns_added);
        REQUIRE(static_cast<size_t>(storage.count<MockDatabasePoco>()) == pocosToInsert.size());

        auto pocosFromDb = storage.get_all<MockDatabasePoco>(order_by(&MockDatabasePoco::id));
        for(size_t i = 0; i < pocosFromDb.size(); ++i) {
            auto& pocoFromDb = pocosFromDb[i];
            auto& oldPoco = pocosToInsert[i];
            REQUIRE(pocoFromDb.id == oldPoco.id);
            REQUIRE(pocoFromDb.name == oldPoco.name);
            REQUIRE(pocoFromDb.alpha == 1);
        }
    }
    // --- Add a new column and delete an old one
    {
        // --- Read the database and create the storage
        auto storage = sqlite_orm::make_storage(
            storagePath,
            sqlite_orm::make_table(
                "pocos",
                sqlite_orm::make_column("id", &MockDatabasePoco::id, sqlite_orm::primary_key()),
                sqlite_orm::make_column("name", &MockDatabasePoco::name),
                sqlite_orm::make_column("beta", &MockDatabasePoco::beta, sqlite_orm::default_value(1.1))));

        // --- We simulate the synchronization first, then do it for real and compare
        auto simulated = storage.sync_schema_simulate(true);
        auto ssr = storage.sync_schema(true);
        REQUIRE(ssr == simulated);
        REQUIRE(ssr["pocos"] == sqlite_orm::sync_schema_result::new_columns_added_and_old_columns_removed);
        REQUIRE(static_cast<size_t>(storage.count<MockDatabasePoco>()) == pocosToInsert.size());

        auto pocosFromDb = storage.get_all<MockDatabasePoco>(order_by(&MockDatabasePoco::id));
        for(size_t i = 0; i < pocosFromDb.size(); ++i) {
            auto& pocoFromDb = pocosFromDb[i];
            auto& oldPoco = pocosToInsert[i];

            REQUIRE(pocoFromDb.id == oldPoco.id);
            REQUIRE(pocoFromDb.name == oldPoco.name);
            REQUIRE(!(pocoFromDb.beta < 1));
        }
    }
}

bool compareUniquePointers(const std::unique_ptr<int>& lhs, const std::unique_ptr<int>& rhs) {
    if(!lhs && !rhs) {
        return true;
    } else {
        if(lhs && rhs) {
            return *lhs == *rhs;
        } else {
            return false;
        }
    }
}

TEST_CASE("sync_schema") {
    using Catch::Matchers::UnorderedEquals;
    struct User {
        int id = 0;
        std::string name;
        int age = 0;
        std::unique_ptr<int> ageNullable;

        User() = default;

        User(int id_) : id(id_) {}

        User(int id_, std::string name_) : id(id_), name(move(name_)) {}

        User(int id_, int age_) : id(id_), age(age_) {}

        User(const User& other) :
            id(other.id), name(other.name), age(other.age),
            ageNullable(other.ageNullable ? std::make_unique<int>(*other.ageNullable) : nullptr) {}

        bool operator==(const User& other) const {
            return this->id == other.id && this->name == other.name && this->age == other.age;
        }
    };
    auto storagePath = "sync_schema.sqlite";
    std::string tableName = "users";
    struct {
        const std::string id = "id";
        const std::string name = "name";
        const std::string age = "age";
    } columnNames;
    ::remove(storagePath);
    {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.name, &User::name)));
        auto syncSchemaSimulateRes = storage.sync_schema_simulate(true);
        auto syncSchemaRes = storage.sync_schema(true);
        REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
        decltype(syncSchemaRes) expected{
            {tableName, sync_schema_result::new_table_created},
        };
        REQUIRE(syncSchemaRes == expected);

        storage.replace(User{1, "Alex"});
        storage.replace(User{2, "Michael"});
    }
    SECTION("remove name column") {
        auto storage =
            make_storage(storagePath, make_table(tableName, make_column(columnNames.id, &User::id, primary_key())));
        SECTION("preserve = true") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            auto syncSchemaRes = storage.sync_schema(true);
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::old_columns_removed},
            };
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
            REQUIRE_THAT(users, UnorderedEquals<User>(std::vector<User>{User{1}, User{2}}));
        }
        SECTION("preserve = false") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate();
            auto syncSchemaRes = storage.sync_schema();
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
            REQUIRE(users.empty());
        }
    }
    SECTION("replace a column with no default value") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::age)));
        std::map<std::string, sync_schema_result> syncSchemaSimulateRes;
        std::map<std::string, sync_schema_result> syncSchemaRes;
        SECTION("preserve = true") {
            syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            syncSchemaRes = storage.sync_schema(true);
        }
        SECTION("preserve = false") {
            syncSchemaSimulateRes = storage.sync_schema_simulate();
            syncSchemaRes = storage.sync_schema();
        }
        REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
        decltype(syncSchemaRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated},
        };
        REQUIRE(syncSchemaRes == expected);
        auto users = storage.get_all<User>();
        REQUIRE(users.empty());
    }
    SECTION("replace a column with default value") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::age, default_value(-1))));
        SECTION("preserve = true") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            auto syncSchemaRes = storage.sync_schema(true);
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::new_columns_added_and_old_columns_removed},
            };
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
            REQUIRE_THAT(users, UnorderedEquals<User>(std::vector<User>{User{1, -1}, User{2, -1}}));
        }
        SECTION("preserve = false") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate();
            auto syncSchemaRes = storage.sync_schema();
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
            REQUIRE(users.empty());
        }
    }
    SECTION("replace a column with null") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::ageNullable)));
        SECTION("preserve = true") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            auto syncSchemaRes = storage.sync_schema(true);
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            {
                decltype(syncSchemaRes) expected{
                    {tableName, sync_schema_result::new_columns_added_and_old_columns_removed},
                };
                REQUIRE(syncSchemaRes == expected);
            }
            auto users = storage.get_all<User>();
            REQUIRE_THAT(users, UnorderedEquals<User>(std::vector<User>{User{1}, User{2}}));
            {
                auto rows = storage.select(asterisk<User>());
                decltype(rows) expected;
                expected.push_back({1, std::unique_ptr<int>()});
                expected.push_back({2, std::unique_ptr<int>()});
                REQUIRE_THAT(rows, UnorderedEquals(expected));
            }
        }
        SECTION("preserve = false") {
            auto syncSchemaSimulateRes = storage.sync_schema_simulate();
            auto syncSchemaRes = storage.sync_schema();
            REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
            REQUIRE(users.empty());
        }
    }
}

TEST_CASE("sync_schema_simulate") {
    struct Cols {
        int Col1;
    };

    auto storage =
        make_storage("db", make_index("IX_Col1", &Cols::Col1), make_table("Table", make_column("Col1", &Cols::Col1)));

    storage.sync_schema();
    storage.sync_schema_simulate();
}
