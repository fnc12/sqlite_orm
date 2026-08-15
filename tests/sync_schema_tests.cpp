#include <cstdint>
#include <cstdio>  //  std::remove
#include <memory>  //  std::unique_ptr, std::make_unique
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

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
        int64 id = 0;
        std::string name;
        std::unique_ptr<int> categoryId;
        std::unique_ptr<std::string> surname;
    };

    //  this is a new version of user
    struct UserAfter {
        int64 id = 0;
        std::string name;

        bool operator==(const UserBefore& before) const {
            return this->id == before.id && this->name == before.name;
        }
    };

    //  create an old storage
    auto filename = "sync_schema_text.sqlite";
    std::remove(filename);
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

    //  create c++ objects to insert into table
    std::vector<UserBefore> usersToInsert;
    usersToInsert.push_back({-1, "Michael", nullptr, std::make_unique<std::string>("Scofield")});
    usersToInsert.push_back({-1, "Lincoln", std::make_unique<int>(4), std::make_unique<std::string>("Burrows")});
    usersToInsert.push_back({-1, "Sucre", nullptr, nullptr});
    usersToInsert.push_back({-1, "Sara", std::make_unique<int>(996), std::make_unique<std::string>("Tancredi")});
    usersToInsert.push_back({-1, "John", std::make_unique<int>(100500), std::make_unique<std::string>("Abruzzi")});
    usersToInsert.push_back({-1, "Brad", std::make_unique<int>(65), nullptr});
    usersToInsert.push_back({-1, "Paul", std::make_unique<int>(65), nullptr});

    for (auto& user: usersToInsert) {
        auto insertedId = storage.insert(user);
        user.id = insertedId;
    }

    //  assert count first cause we shall be asserting row by row next
    REQUIRE(static_cast<size_t>(storage.count<UserBefore>()) == usersToInsert.size());

    //  now we create a new storage with a partial schema
    auto newStorage = make_storage(
        filename,
        make_table("users", make_column("id", &UserAfter::id, primary_key()), make_column("name", &UserAfter::name)));
    SECTION("preserve = false") {
        syncSchemaSimulationRes = newStorage.sync_schema_simulate(false);
        syncSchemaRes = newStorage.sync_schema(false);

        REQUIRE(syncSchemaRes.size() == 1);
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
        REQUIRE(syncSchemaRes.begin()->second == sync_schema_result::old_columns_removed);
#else
        REQUIRE(syncSchemaRes.begin()->second == sync_schema_result::dropped_and_recreated);
#endif
        REQUIRE(syncSchemaSimulationRes == syncSchemaRes);
    }
    SECTION("preserve = true") {
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

        for (size_t i = 0; i < usersFromDb.size(); ++i) {
            auto& userFromDb = usersFromDb[i];
            auto& oldUser = usersToInsert[i];
            REQUIRE(userFromDb == oldUser);
        }

        auto usersCountBefore = newStorage.count<UserAfter>();

        syncSchemaSimulationRes = newStorage.sync_schema_simulate();
        syncSchemaRes = newStorage.sync_schema();
        REQUIRE(syncSchemaRes == syncSchemaSimulationRes);

        auto usersCountAfter = newStorage.count<UserAfter>();
        REQUIRE(usersCountBefore == usersCountAfter);
    }
}

TEST_CASE("issue854") {
    struct Base {
        std::string name;
        int64_t timestamp = 0;
        int64_t value = 0;
    };

    struct A : public Base {
        int64_t id = 0;
    };
    auto storage = make_storage({},
                                make_table("entries",
                                           make_column("id", &A::id, sqlite_orm::primary_key().autoincrement()),
                                           make_column("name", &A::name),
                                           make_column("timestamp", &A::timestamp),
                                           unique(column<A>(&Base::name), column<A>(&Base::timestamp))));
    storage.sync_schema();
}

TEST_CASE("issue521") {
    auto storagePath = "issue521.sqlite";

    struct MockDatabasePoco {
        int64 id = 0;
        std::string name;
        std::uint32_t alpha{0};
        float beta{0.0};
    };
    std::vector<MockDatabasePoco> pocosToInsert;

    std::remove(storagePath);
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
        pocosToInsert.push_back({0, "Michael", 10, 10.10f});
        pocosToInsert.push_back({0, "Joyce", 20, 20.20f});

        for (auto& poco: pocosToInsert) {
            auto insertedId = storage.insert(poco);
            poco.id = insertedId;
        }

        // --- Retrieve the pocos and verify
        REQUIRE(static_cast<size_t>(storage.count<MockDatabasePoco>()) == pocosToInsert.size());

        using namespace sqlite_orm;
        auto pocosFromDb = storage.get_all<MockDatabasePoco>(order_by(&MockDatabasePoco::id));
        for (size_t i = 0; i < pocosFromDb.size(); ++i) {
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
        for (size_t i = 0; i < pocosFromDb.size(); ++i) {
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
        for (size_t i = 0; i < pocosFromDb.size(); ++i) {
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
        for (size_t i = 0; i < pocosFromDb.size(); ++i) {
            auto& pocoFromDb = pocosFromDb[i];
            auto& oldPoco = pocosToInsert[i];

            REQUIRE(pocoFromDb.id == oldPoco.id);
            REQUIRE(pocoFromDb.name == oldPoco.name);
            REQUIRE_FALSE((pocoFromDb.beta < 1));
        }
    }
}

bool compareUniquePointers(const std::unique_ptr<int>& lhs, const std::unique_ptr<int>& rhs) {
    if (!lhs && !rhs) {
        return true;
    } else {
        if (lhs && rhs) {
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

        User(int id_, std::string name_) : id(id_), name(std::move(name_)) {}

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
    std::remove(storagePath);
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
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::old_columns_removed},
            };
#else
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
#endif
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            REQUIRE_FALSE(users.empty());
#else
            REQUIRE(users.empty());
#endif
        }
    }
    SECTION("replace a column with no default value") {
        auto storage = make_storage(storagePath,
                                    make_table(tableName,
                                               make_column(columnNames.id, &User::id, primary_key()),
                                               make_column(columnNames.age, &User::age)));
        std::map<std::string, sync_schema_result> syncSchemaSimulateRes;
        std::map<std::string, sync_schema_result> syncSchemaRes;
        SECTION(
            "preserve = true") {  // there is NO way we can preserve data by adding a column with no default value and not nullable!
            syncSchemaSimulateRes = storage.sync_schema_simulate(true);
            syncSchemaRes = storage.sync_schema(true);
        }
        SECTION("preserve = false") {
            syncSchemaSimulateRes = storage.sync_schema_simulate();
            syncSchemaRes = storage.sync_schema();
        }
        REQUIRE(syncSchemaSimulateRes == syncSchemaRes);
        decltype(syncSchemaRes) expected{
            {tableName, sync_schema_result::dropped_and_recreated_with_data_loss},
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
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::new_columns_added_and_old_columns_removed},
            };
#else
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
#endif
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            REQUIRE_FALSE(users.empty());
#else
            REQUIRE(users.empty());
#endif
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
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::new_columns_added_and_old_columns_removed},
            };
#else
            decltype(syncSchemaRes) expected{
                {tableName, sync_schema_result::dropped_and_recreated},
            };
#endif
            REQUIRE(syncSchemaRes == expected);
            auto users = storage.get_all<User>();
#if SQLITE_VERSION_NUMBER >= 3035000  //  DROP COLUMN feature exists (v3.35.0)
            REQUIRE_FALSE(users.empty());
#else
            REQUIRE(users.empty());
#endif
        }
    }
}

// https://github.com/fnc12/sqlite_orm/issues/1462
TEST_CASE("Distinguish dropped_and_recreated with and without backup") {
    struct MyTableRecord {
        int id = 0;
        std::string name;
    };

    auto storagePath = "issue1462.sqlite";
    std::remove(storagePath);

    SECTION("sequential schema changes (exact issue scenario)") {
        // Step 1: create initial table with a single column, insert data
        {
            auto storage = make_storage(storagePath, make_table("MyTable", make_column("id", &MyTableRecord::id)));
            storage.sync_schema(true);
            storage.insert(MyTableRecord{1, ""});
            storage.insert(MyTableRecord{2, ""});
        }
        // Step 2: add primary key — data should be preserved
        {
            auto storage =
                make_storage(storagePath, make_table("MyTable", make_column("id", &MyTableRecord::id, primary_key())));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("MyTable") == sync_schema_result::dropped_and_recreated);

            auto allRecords = storage.get_all<MyTableRecord>();
            REQUIRE(allRecords.size() == 2);
        }
        // Step 3: add NOT NULL column without default — data should be lost
        {
            auto storage = make_storage(storagePath,
                                        make_table("MyTable",
                                                   make_column("id", &MyTableRecord::id, primary_key()),
                                                   make_column("name", &MyTableRecord::name)));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("MyTable") == sync_schema_result::dropped_and_recreated_with_data_loss);

            auto allRecords = storage.get_all<MyTableRecord>();
            REQUIRE(allRecords.empty());
        }
    }

    SECTION("combined pk change and new NOT NULL column in one step") {
        // create initial table with a single column, insert data
        {
            auto storage = make_storage(storagePath, make_table("MyTable", make_column("id", &MyTableRecord::id)));
            storage.sync_schema(true);
            storage.insert(MyTableRecord{1, ""});
            storage.insert(MyTableRecord{2, ""});
        }
        // add primary key AND NOT NULL column without default at once — data should be lost
        {
            auto storage = make_storage(storagePath,
                                        make_table("MyTable",
                                                   make_column("id", &MyTableRecord::id, primary_key()),
                                                   make_column("name", &MyTableRecord::name)));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("MyTable") == sync_schema_result::dropped_and_recreated_with_data_loss);

            auto allRecords = storage.get_all<MyTableRecord>();
            REQUIRE(allRecords.empty());
        }
    }

    std::remove(storagePath);
}

TEST_CASE("sync_schema_simulate") {
    struct Cols {
        int Col1 = 0;
    };

    auto storage =
        make_storage("db", make_index("IX_Col1", &Cols::Col1), make_table("Table", make_column("Col1", &Cols::Col1)));

    storage.sync_schema();
    storage.sync_schema_simulate();

    std::remove("db");
}

#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("sync_schema with generated columns") {
    struct User {
        int id = 0;
        int hash = 0;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const User& other) const = default;
#else
        bool operator==(const User& other) const {
            return this->id == other.id && this->hash == other.hash;
        }
#endif
    };
    auto storagePath = "sync_schema_with_generated.sqlite";
    std::remove(storagePath);
    auto storage1 = make_storage(storagePath, make_table("users", make_column("id", &User::id)));
    storage1.sync_schema();
    storage1.insert(User{5});
    SECTION("add a generated column and sync schema with preserve = false") {
        auto generatedAlwaysConstraint = generated_always_as(c(&User::id) + 4);
        std::vector<User> allUsers;
        decltype(allUsers) expectedUsers;
        SECTION("virtual") {
            generatedAlwaysConstraint = std::move(generatedAlwaysConstraint).virtual_();
            expectedUsers.push_back({5, 9});
        }
        SECTION("not specified") {
            expectedUsers.push_back({5, 9});
        }
        SECTION("stored") {
            generatedAlwaysConstraint = std::move(generatedAlwaysConstraint).stored();
            // with preserve == false nothing is preserved since this kind of generated column requires dropping the table
            // thus we don't expect any users to be preserved!
        }
        auto storage2 = make_storage(storagePath,
                                     make_table("users",
                                                make_column("id", &User::id),
                                                make_column("hash", &User::hash, generatedAlwaysConstraint)));
        storage2.sync_schema();
        allUsers = storage2.get_all<User>();
        REQUIRE(allUsers == expectedUsers);
    }
    SECTION("add a generated column and sync schema with preserve = true") {
        auto generatedAlwaysConstraint = generated_always_as(c(&User::id) + 4);
        std::vector<User> allUsers;
        decltype(allUsers) expectedUsers;
        SECTION("not specified") {
            expectedUsers.push_back({5, 9});
        }
        SECTION("virtual") {
            generatedAlwaysConstraint = std::move(generatedAlwaysConstraint).virtual_();
            expectedUsers.push_back({5, 9});
        }
        SECTION("stored") {
            generatedAlwaysConstraint = std::move(generatedAlwaysConstraint).stored();
            expectedUsers.push_back({5, 9});
        }
        auto storage2 = make_storage(storagePath,
                                     make_table("users",
                                                make_column("id", &User::id),
                                                make_column("hash", &User::hash, generatedAlwaysConstraint)));
        storage2.sync_schema(true);
        allUsers = storage2.get_all<User>();
        REQUIRE(allUsers == expectedUsers);
    }
}
#endif

/**
 *  These tests pin down `sync_schema` behavior for every column attribute which takes part
 *  in the schema comparison (existence, NOT NULL, default value presence, primary key
 *  membership/order, generated flag) as well as for every attribute which is deliberately
 *  NOT compared (column type, default value itself, generated expression and its storage kind,
 *  UNIQUE, CHECK, COLLATE, WITHOUT ROWID).
 *  Every scenario runs on the same database file so that a real migration takes place,
 *  and asserts the `sync_schema_simulate` == `sync_schema` invariant.
 */

TEST_CASE("sync_schema attribute matrix: NOT NULL toggle") {
    struct UserNullable {
        int id = 0;
        std::unique_ptr<int> age;
    };
    struct UserNotNull {
        int id = 0;
        int age = 0;
    };
    auto storagePath = "sync_schema_matrix_notnull.sqlite";
    std::remove(storagePath);
    SECTION("nullable -> NOT NULL") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &UserNullable::id, primary_key()),
                                                   make_column("age", &UserNullable::age)));
            storage.sync_schema();
            UserNullable user{1, std::make_unique<int>(42)};
            storage.replace(user);
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserNotNull::id, primary_key()),
                                               make_column("age", &UserNotNull::age)));
        SECTION("preserve = true") {
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserNotNull>();
            REQUIRE(users.size() == 1);
            REQUIRE(users.front().age == 42);
        }
        SECTION("preserve = false") {
            auto simulateRes = storage.sync_schema_simulate();
            auto syncRes = storage.sync_schema();
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            REQUIRE(storage.count<UserNotNull>() == 0);
        }
    }
    SECTION("NOT NULL -> nullable") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &UserNotNull::id, primary_key()),
                                                   make_column("age", &UserNotNull::age)));
            storage.sync_schema();
            storage.replace(UserNotNull{1, 42});
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserNullable::id, primary_key()),
                                               make_column("age", &UserNullable::age)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserNullable>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().age);
        REQUIRE(*users.front().age == 42);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: default value presence toggle") {
    struct User {
        int id = 0;
        int score = 0;
    };
    auto storagePath = "sync_schema_matrix_default.sqlite";
    std::remove(storagePath);
    SECTION("no default -> default") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users", make_column("id", &User::id, primary_key()), make_column("score", &User::score)));
            storage.sync_schema();
            storage.replace(User{1, 10});
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("score", &User::score, default_value(42))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().score == 10);
    }
    SECTION("default -> no default") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &User::id, primary_key()),
                                                   make_column("score", &User::score, default_value(42))));
            storage.sync_schema();
            storage.replace(User{1, 10});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("score", &User::score)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().score == 10);
    }
    SECTION("default value change is NOT detected") {
        {
            auto storage = make_storage(storagePath,
                                        make_table("users",
                                                   make_column("id", &User::id, primary_key()),
                                                   make_column("score", &User::score, default_value(1))));
            storage.sync_schema();
        }
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("score", &User::score, default_value(2))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  only the presence of a default value is compared, not the value itself
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: column type change is NOT detected") {
    struct UserInt {
        int id = 0;
        int data = 0;
    };
    struct UserText {
        int id = 0;
        std::string data;
    };
    auto storagePath = "sync_schema_matrix_type.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserInt::id, primary_key()), make_column("data", &UserInt::data)));
        storage.sync_schema();
        storage.replace(UserInt{1, 42});
    }
    auto storage = make_storage(
        storagePath,
        make_table("users", make_column("id", &UserText::id, primary_key()), make_column("data", &UserText::data)));
    auto simulateRes = storage.sync_schema_simulate(true);
    auto syncRes = storage.sync_schema(true);
    REQUIRE(simulateRes == syncRes);
    //  the column type is deliberately not compared (removed in 2020)
    REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    std::remove(storagePath);
}

TEST_CASE("sync_schema attribute matrix: composite primary key") {
    struct Record {
        int year = 0;
        int month = 0;
        int amount = 0;

#ifdef SQLITE_ORM_DEFAULT_COMPARISONS_SUPPORTED
        bool operator==(const Record&) const = default;
#else
        bool operator==(const Record& other) const {
            return this->year == other.year && this->month == other.month && this->amount == other.amount;
        }
#endif
    };
    auto storagePath = "sync_schema_matrix_pk.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::year, &Record::month)));
        storage.sync_schema();
        storage.replace(Record{2026, 8, 100});
    }
    SECTION("reordering columns within the primary key is detected") {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::month, &Record::year)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::dropped_and_recreated);
        auto records = storage.get_all<Record>();
        REQUIRE(records == std::vector<Record>{Record{2026, 8, 100}});
    }
    SECTION("extending the primary key is detected") {
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &Record::year),
                                               make_column("month", &Record::month),
                                               make_column("amount", &Record::amount),
                                               primary_key(&Record::year, &Record::month, &Record::amount)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::dropped_and_recreated);
        auto records = storage.get_all<Record>();
        REQUIRE(records == std::vector<Record>{Record{2026, 8, 100}});
    }
    std::remove(storagePath);
}

#if SQLITE_VERSION_NUMBER >= 3031000
TEST_CASE("sync_schema attribute matrix: generated columns") {
    struct User {
        int id = 0;
        int hash = 0;
    };
    auto storagePath = "sync_schema_matrix_generated.sqlite";
    std::remove(storagePath);
    SECTION("regular column becomes generated") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users", make_column("id", &User::id, primary_key()), make_column("hash", &User::hash)));
            storage.sync_schema();
            storage.replace(User{5, 1000});
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        //  the value is recomputed by the generated column expression
        REQUIRE(users.front().hash == 9);
    }
    SECTION("generated column becomes regular") {
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &User::id, primary_key()),
                                        make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
            storage.sync_schema();
            storage.replace(User{5, 0});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("hash", &User::hash)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<User>();
        REQUIRE(users.size() == 1);
        //  the formerly generated value is materialized into the regular column
        REQUIRE(users.front().hash == 9);
    }
    SECTION("VIRTUAL <-> STORED change is NOT detected") {
        {
            auto storage = make_storage(
                storagePath,
                make_table("users",
                           make_column("id", &User::id, primary_key()),
                           make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4).virtual_())));
            storage.sync_schema();
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4).stored())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  only the fact that a column is generated is compared, not its storage kind
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("generated expression change is NOT detected") {
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &User::id, primary_key()),
                                        make_column("hash", &User::hash, generated_always_as(c(&User::id) + 4))));
            storage.sync_schema();
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &User::id, primary_key()),
                                    make_column("hash", &User::hash, generated_always_as(c(&User::id) + 5))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        //  the generated column expression is not compared
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}
#endif

TEST_CASE("sync_schema attribute matrix: constraints which are NOT compared") {
    struct User {
        int id = 0;
        std::string email;
    };
    auto storagePath = "sync_schema_matrix_constraints.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("email", &User::email)));
        storage.sync_schema();
    }
    SECTION("adding UNIQUE is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, unique())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding CHECK is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, check(length(&User::email) > 0))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding COLLATE is NOT detected") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &User::id, primary_key()),
                                               make_column("email", &User::email, collate_nocase())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    SECTION("adding WITHOUT ROWID is NOT detected") {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("email", &User::email))
                .without_rowid());
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::already_in_sync);
    }
    std::remove(storagePath);
}

/**
 *  SQLite forbids `ALTER TABLE ... ADD COLUMN` for columns with a PRIMARY KEY or UNIQUE
 *  constraint and for columns with a non-constant default value. `sync_schema` must
 *  classify such new columns as `dropped_and_recreated` (going through the backup table)
 *  instead of attempting `ADD COLUMN` and failing at runtime.
 */
TEST_CASE("sync_schema new column edge cases") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storagePath = "sync_schema_matrix_add_column.sqlite";
    std::remove(storagePath);
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
        storage.sync_schema();
        storage.replace(User{1, "Michael"});
        storage.replace(User{2, "Lincoln"});
    }
    SECTION("new column with a column-level UNIQUE constraint") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::unique_ptr<std::string> email;
        };
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserV2::id, primary_key()),
                                               make_column("name", &UserV2::name),
                                               make_column("email", &UserV2::email, unique())));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
    }
    SECTION("new column referenced by a table-level UNIQUE constraint") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::unique_ptr<std::string> email;
        };
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserV2::id, primary_key()),
                                               make_column("name", &UserV2::name),
                                               make_column("email", &UserV2::email),
                                               unique(&UserV2::email)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
        //  the UNIQUE constraint must actually be in effect now
        UserV2 user3{0, "Sucre", std::make_unique<std::string>("email@example.org")};
        storage.insert(user3);
        UserV2 user4{0, "Sara", std::make_unique<std::string>("email@example.org")};
        REQUIRE_THROWS(storage.insert(user4));
    }
    SECTION("new column with a non-constant default value") {
        struct UserV2 {
            int id = 0;
            std::string name;
            std::string created;
        };
        SECTION("CURRENT_TIMESTAMP") {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &UserV2::id, primary_key()),
                                        make_column("name", &UserV2::name),
                                        make_column("created", &UserV2::created, default_value(current_timestamp()))));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserV2>();
            REQUIRE(users.size() == 2);
        }
        SECTION("expression") {
            auto storage = make_storage(
                storagePath,
                make_table("users",
                           make_column("id", &UserV2::id, primary_key()),
                           make_column("name", &UserV2::name),
                           make_column("created", &UserV2::created, default_value(datetime("now", "localtime")))));
            auto simulateRes = storage.sync_schema_simulate(true);
            auto syncRes = storage.sync_schema(true);
            REQUIRE(simulateRes == syncRes);
            REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated);
            auto users = storage.get_all<UserV2>();
            REQUIRE(users.size() == 2);
        }
    }
    SECTION("new INTEGER PRIMARY KEY column") {
        struct EventV1 {
            int value = 0;
        };
        struct EventV2 {
            std::unique_ptr<int> id;
            int value = 0;
        };
        auto eventsPath = "sync_schema_matrix_add_pk_column.sqlite";
        std::remove(eventsPath);
        {
            auto storage = make_storage(eventsPath, make_table("events", make_column("value", &EventV1::value)));
            storage.sync_schema();
            storage.insert(EventV1{10});
            storage.insert(EventV1{20});
        }
        auto storage = make_storage(eventsPath,
                                    make_table("events",
                                               make_column("id", &EventV2::id, primary_key()),
                                               make_column("value", &EventV2::value)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("events") == sync_schema_result::dropped_and_recreated);
        auto events = storage.get_all<EventV2>();
        REQUIRE(events.size() == 2);
        std::remove(eventsPath);
    }
#if SQLITE_VERSION_NUMBER >= 3031000
    SECTION("new STORED generated column together with a new NOT NULL column without default") {
        struct UserV2 {
            int id = 0;
            std::string name;
            int hash = 0;
            std::string token;
        };
        //  the STORED generated column comes first on purpose: scanning of new columns must not
        //  stop on it and must still discover the NOT NULL column which makes data preservation impossible
        auto storage = make_storage(
            storagePath,
            make_table("users",
                       make_column("id", &UserV2::id, primary_key()),
                       make_column("name", &UserV2::name),
                       make_column("hash", &UserV2::hash, generated_always_as(c(&UserV2::id) + 4).stored()),
                       make_column("token", &UserV2::token)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::dropped_and_recreated_with_data_loss);
        REQUIRE(storage.count<UserV2>() == 0);
    }
#endif
    std::remove(storagePath);
}

/**
 *  SQLite's `ALTER TABLE ... DROP COLUMN` refuses to drop a column which is part of the
 *  primary key, has a UNIQUE constraint, is indexed or is referenced in a generated column,
 *  index, trigger or view expression. `sync_schema` must fall back to recreating the table
 *  through a backup table in such cases instead of failing at runtime.
 */
TEST_CASE("sync_schema removed column edge cases") {
    SECTION("removed column is referenced by an index") {
        struct UserV1 {
            int id = 0;
            std::string name;
            int category = 0;
        };
        struct UserV2 {
            int id = 0;
            std::string name;
        };
        auto storagePath = "sync_schema_matrix_drop_indexed.sqlite";
        std::remove(storagePath);
        {
            auto storage = make_storage(storagePath,
                                        make_index("idx_users_category", &UserV1::category),
                                        make_table("users",
                                                   make_column("id", &UserV1::id, primary_key()),
                                                   make_column("name", &UserV1::name),
                                                   make_column("category", &UserV1::category)));
            storage.sync_schema();
            storage.replace(UserV1{1, "Michael", 10});
            storage.replace(UserV1{2, "Lincoln", 20});
        }
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserV2::id, primary_key()), make_column("name", &UserV2::name)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::old_columns_removed);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 2);
        std::remove(storagePath);
    }
    SECTION("removed column is part of the primary key") {
        struct RecordV1 {
            int year = 0;
            int month = 0;
            int amount = 0;
        };
        struct RecordV2 {
            int year = 0;
            int amount = 0;
        };
        auto storagePath = "sync_schema_matrix_drop_pk_member.sqlite";
        std::remove(storagePath);
        {
            auto storage = make_storage(storagePath,
                                        make_table("records",
                                                   make_column("year", &RecordV1::year),
                                                   make_column("month", &RecordV1::month),
                                                   make_column("amount", &RecordV1::amount),
                                                   primary_key(&RecordV1::year, &RecordV1::month)));
            storage.sync_schema();
            storage.replace(RecordV1{2025, 8, 100});
            storage.replace(RecordV1{2026, 8, 200});
        }
        auto storage = make_storage(storagePath,
                                    make_table("records",
                                               make_column("year", &RecordV2::year),
                                               make_column("amount", &RecordV2::amount),
                                               primary_key(&RecordV2::year)));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("records") == sync_schema_result::old_columns_removed);
        auto records = storage.get_all<RecordV2>();
        REQUIRE(records.size() == 2);
        std::remove(storagePath);
    }
#if SQLITE_VERSION_NUMBER >= 3031000
    SECTION("extra db column is removed while a generated column is present") {
        //  on SQLite < 3.35 this goes through the backup table, which must not try
        //  to copy the generated column
        struct UserV1 {
            int id = 0;
            int hash = 0;
            std::string obsolete;
        };
        struct UserV2 {
            int id = 0;
            int hash = 0;
        };
        auto storagePath = "sync_schema_matrix_drop_with_generated.sqlite";
        std::remove(storagePath);
        {
            auto storage =
                make_storage(storagePath,
                             make_table("users",
                                        make_column("id", &UserV1::id, primary_key()),
                                        make_column("hash", &UserV1::hash, generated_always_as(c(&UserV1::id) + 4)),
                                        make_column("obsolete", &UserV1::obsolete, default_value(""))));
            storage.sync_schema();
            UserV1 user{5, 0, ""};
            storage.replace(user);
        }
        auto storage =
            make_storage(storagePath,
                         make_table("users",
                                    make_column("id", &UserV2::id, primary_key()),
                                    make_column("hash", &UserV2::hash, generated_always_as(c(&UserV2::id) + 4))));
        auto simulateRes = storage.sync_schema_simulate(true);
        auto syncRes = storage.sync_schema(true);
        REQUIRE(simulateRes == syncRes);
        REQUIRE(syncRes.at("users") == sync_schema_result::old_columns_removed);
        auto users = storage.get_all<UserV2>();
        REQUIRE(users.size() == 1);
        REQUIRE(users.front().hash == 9);
        std::remove(storagePath);
    }
#endif
}

/**
 *  SQLite validates foreign key targets lazily (at DML time, not at `CREATE TABLE` time),
 *  so two tables with cross-referencing foreign keys can be created in ANY order.
 *  These tests prove it with foreign key enforcement enabled
 *  (sqlite_orm turns `PRAGMA foreign_keys` on automatically when the schema contains foreign keys).
 */
TEST_CASE("sync_schema with cross-referencing foreign keys") {
    struct CrossArtist {
        int id = 0;
        std::unique_ptr<int> lastAlbumId;
    };
    struct CrossAlbum {
        int id = 0;
        std::unique_ptr<int> artistId;
    };
    auto storagePath = "sync_schema_cross_fk.sqlite";
    std::remove(storagePath);

    auto artistsTable = [] {
        return make_table("artists",
                          make_column("id", &CrossArtist::id, primary_key()),
                          make_column("last_album_id", &CrossArtist::lastAlbumId),
                          foreign_key(&CrossArtist::lastAlbumId).references(&CrossAlbum::id));
    };
    auto albumsTable = [] {
        return make_table("albums",
                          make_column("id", &CrossAlbum::id, primary_key()),
                          make_column("artist_id", &CrossAlbum::artistId),
                          foreign_key(&CrossAlbum::artistId).references(&CrossArtist::id));
    };

    auto requireForeignKeysWork = [](auto& storage) {
        //  a valid insert sequence: the nullable foreign keys break the chicken-and-egg problem
        CrossArtist artist;
        artist.id = 1;
        storage.replace(artist);
        CrossAlbum album;
        album.id = 10;
        album.artistId = std::make_unique<int>(1);
        storage.replace(album);
        storage.update_all(set(c(&CrossArtist::lastAlbumId) = 10), where(c(&CrossArtist::id) == 1));

        //  the foreign keys are actually enforced
        CrossAlbum orphanAlbum;
        orphanAlbum.id = 11;
        orphanAlbum.artistId = std::make_unique<int>(777);
        REQUIRE_THROWS(storage.replace(orphanAlbum));
    };

    SECTION("artists declared first") {
        auto storage = make_storage(storagePath, artistsTable(), albumsTable());
        auto simulateResult = storage.sync_schema_simulate();
        auto syncResult = storage.sync_schema();
        REQUIRE(simulateResult == syncResult);
        REQUIRE(syncResult.at("artists") == sync_schema_result::new_table_created);
        REQUIRE(syncResult.at("albums") == sync_schema_result::new_table_created);
        requireForeignKeysWork(storage);
    }
    SECTION("albums declared first") {
        auto storage = make_storage(storagePath, albumsTable(), artistsTable());
        auto simulateResult = storage.sync_schema_simulate();
        auto syncResult = storage.sync_schema();
        REQUIRE(simulateResult == syncResult);
        REQUIRE(syncResult.at("artists") == sync_schema_result::new_table_created);
        REQUIRE(syncResult.at("albums") == sync_schema_result::new_table_created);
        requireForeignKeysWork(storage);
    }
    std::remove(storagePath);
}

/**
 *  Database objects must be synced in dependency order regardless of the declaration order:
 *  an index has to be created after its table, and recreated after its table has been
 *  recreated through the backup-table procedure (which drops the table's indexes).
 */
TEST_CASE("sync_schema syncs objects in dependency order") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct UserV2 {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    auto storagePath = "sync_schema_dependency_order.sqlite";
    std::remove(storagePath);

    auto indexCount = [&storagePath] {
        auto inspectionStorage = make_storage(storagePath, make_sqlite_schema_table());
        return inspectionStorage.count<sqlite_master>(
            where(c(&sqlite_master::type) == "index" and c(&sqlite_master::name) == "idx_users_name"));
    };

    SECTION("a new table and its index are created from a straight declaration order") {
        //  the order sqlite2orm generates: the table first, its index second
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_index("idx_users_name", &User::name));
        storage.sync_schema();
        REQUIRE(indexCount() == 1);
    }
    SECTION("the index is recreated after its table has been recreated") {
        {
            auto storage = make_storage(
                storagePath,
                make_index("idx_users_name", &User::name),
                make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
            storage.sync_schema();
            storage.replace(User{1, "Leony"});
            REQUIRE(indexCount() == 1);
        }
        //  straight declaration order + a column change which makes sync_schema recreate the table
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserV2::id, primary_key()), make_column("name", &UserV2::name)),
            make_index("idx_users_name", &UserV2::name));
        auto syncResult = storage.sync_schema(true);
        REQUIRE(syncResult.at("users") == sync_schema_result::dropped_and_recreated);
        REQUIRE(indexCount() == 1);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema dependency order state matrix") {
    struct User {
        int id = 0;
        std::string name;
    };
    struct UserV2 {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    struct UserWithEmail {
        int id = 0;
        std::string name;
        std::unique_ptr<std::string> email;
    };
    auto storagePath = "sync_schema_dependency_matrix.sqlite";
    std::remove(storagePath);

    auto objectCount = [&storagePath](const std::string& type, const std::string& name) {
        auto inspectionStorage = make_storage(storagePath, make_sqlite_schema_table());
        return inspectionStorage.count<sqlite_master>(
            where(c(&sqlite_master::type) == type and c(&sqlite_master::name) == name));
    };

    //  the common starting point: a table with an index and a trigger, everything in sync
    {
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
            make_index("idx_users_name", &User::name),
            make_trigger("trg_users", after().insert().on<User>().begin(update_all(set(c(&User::name) = "updated")))));
        storage.sync_schema();
        storage.replace(User{1, "Leony"});
        REQUIRE(objectCount("index", "idx_users_name") == 1);
        REQUIRE(objectCount("trigger", "trg_users") == 1);
    }
    SECTION("an in-place column addition keeps the dependent objects") {
        auto storage = make_storage(storagePath,
                                    make_table("users",
                                               make_column("id", &UserWithEmail::id, primary_key()),
                                               make_column("name", &UserWithEmail::name),
                                               make_column("email", &UserWithEmail::email)),
                                    make_index("idx_users_name", &UserWithEmail::name),
                                    make_trigger("trg_users",
                                                 after().insert().on<UserWithEmail>().begin(
                                                     update_all(set(c(&UserWithEmail::name) = "updated")))));
        auto syncResult = storage.sync_schema(true);
        REQUIRE(syncResult.at("users") == sync_schema_result::new_columns_added);
        REQUIRE(objectCount("index", "idx_users_name") == 1);
        REQUIRE(objectCount("trigger", "trg_users") == 1);
    }
    SECTION("dependent objects are recreated after the table has been recreated") {
        //  a NOT NULL change makes sync_schema recreate the table through the backup procedure,
        //  which destroys its indexes and triggers
        auto storage = make_storage(
            storagePath,
            make_table("users", make_column("id", &UserV2::id, primary_key()), make_column("name", &UserV2::name)),
            make_index("idx_users_name", &UserV2::name),
            make_trigger("trg_users", after().insert().on<UserV2>().begin(update_all(set(c(&UserV2::id) = 100)))));
        auto syncResult = storage.sync_schema(true);
        REQUIRE(syncResult.at("users") == sync_schema_result::dropped_and_recreated);
        REQUIRE(objectCount("index", "idx_users_name") == 1);
        REQUIRE(objectCount("trigger", "trg_users") == 1);
    }
    std::remove(storagePath);
}

TEST_CASE("sync_schema dependency order compositions") {
    struct Employee {
        int id = 0;
        std::unique_ptr<int> managerId;
    };
    struct Product {
        int id = 0;
        std::string title;
    };
    auto storagePath = "sync_schema_dependency_compositions.sqlite";
    std::remove(storagePath);

    SECTION("a self-referencing foreign key") {
        auto storage = make_storage(storagePath,
                                    make_table("employees",
                                               make_column("id", &Employee::id, primary_key()),
                                               make_column("manager_id", &Employee::managerId),
                                               foreign_key(&Employee::managerId).references(&Employee::id)));
        auto syncResult = storage.sync_schema();
        REQUIRE(syncResult.at("employees") == sync_schema_result::new_table_created);
        Employee manager;
        manager.id = 1;
        storage.replace(manager);
        Employee worker;
        worker.id = 2;
        worker.managerId = std::make_unique<int>(1);
        storage.replace(worker);
        REQUIRE(storage.count<Employee>() == 2);
    }
    SECTION("interleaved declaration: every index before its own table") {
        auto storage = make_storage(storagePath,
                                    make_index("idx_products_title", &Product::title),
                                    make_table("employees",
                                               make_column("id", &Employee::id, primary_key()),
                                               make_column("manager_id", &Employee::managerId)),
                                    make_index("idx_employees_manager", &Employee::managerId),
                                    make_table("products",
                                               make_column("id", &Product::id, primary_key()),
                                               make_column("title", &Product::title)));
        storage.sync_schema();
        auto inspectionStorage = make_storage(storagePath, make_sqlite_schema_table());
        REQUIRE(inspectionStorage.count<sqlite_master>(
                    where(c(&sqlite_master::type) == "index" and like(&sqlite_master::name, "idx_%"))) == 2);
    }
    std::remove(storagePath);
}
