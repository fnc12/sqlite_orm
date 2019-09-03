#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

/**
 *  this is the deal: assume we have a `users` table with schema
 *  `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL, category_id INTEGER, surname TEXT)`.
 *  We create a storage and insert several objects. Next we simulate schema changing (app update for example): create
 *  another storage with a new schema partial of the previous one: `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL)`.
 *  Next we call `sync_schema(true)` and assert that all users are saved. This test tests whether REMOVE COLUMN imitation works well.
 */
TEST_CASE("Sync schema"){
    
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
    
    for(auto &user : usersToInsert) {
        auto insertedId = storage.insert(user);
        user.id = insertedId;
    }
    
    //  assert count first cause we shall be asserting row by row next
    REQUIRE(static_cast<size_t>(storage.count<UserBefore>()) == usersToInsert.size());
    
    //  now we create a new storage with a partial schema
    auto newStorage = make_storage(filename,
                                   make_table("users",
                                              make_column("id", &UserAfter::id, primary_key()),
                                              make_column("name", &UserAfter::name)));
    
    syncSchemaSimulationRes = newStorage.sync_schema_simulate(true);
    
    //  now call `sync_schema` with argument `preserve` as `true`. It will retain the data in case `sqlite_orm` needs to remove a column
    syncSchemaRes = newStorage.sync_schema(true);
    REQUIRE(syncSchemaRes.size() == 1);
    REQUIRE(syncSchemaRes.begin()->second == sync_schema_result::old_columns_removed);
    REQUIRE(syncSchemaSimulationRes == syncSchemaRes);
    
    //  get all users after syncing the schema
    auto usersFromDb = newStorage.get_all<UserAfter>(order_by(&UserAfter::id));
    
    REQUIRE(usersFromDb.size() == usersToInsert.size());
    
    for(size_t i = 0; i < usersFromDb.size(); ++i) {
        auto &userFromDb = usersFromDb[i];
        auto &oldUser = usersToInsert[i];
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
    std::transform(users.begin(),
                   users.end(),
                   std::back_inserter(idsFromGetAll),
                   [=](auto &user) {
                       return user.id;
                   });
    REQUIRE(std::equal(ids.begin(),
                       ids.end(),
                       idsFromGetAll.begin(),
                       idsFromGetAll.end()));
    
}
