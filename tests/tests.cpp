//
//  tests.cpp
//  CPPTest
//
//  Created by John Zakharov on 05.01.17.
//  Copyright © 2017 John Zakharov. All rights reserved.
//

//#include "tests.hpp"

#include "sqlite_orm.h"

#include <cassert>
#include <vector>

using namespace sqlite_orm;

void testTypeParsing() {
    
    //  int
    assert(*to_sqlite_type("INT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("integeer") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INTEGER") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("TINYINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("SMALLINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("MEDIUMINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("BIGINT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("UNSIGNED BIG INT") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INT2") == sqlite_type::INTEGER);
    assert(*to_sqlite_type("INT8") == sqlite_type::INTEGER);
    
    //  text
    assert(*to_sqlite_type("TEXT") == sqlite_type::TEXT);
    assert(*to_sqlite_type("CLOB") == sqlite_type::TEXT);
    //    assert(*to_sqlite_type("CHARACTER()") == sqlite_type::TEXT);
    for(auto i = 0; i< 255; ++i) {
        //        auto sqliteTypeString = "CHARACTER(" + std::to_string(i) + ")";
        assert(*to_sqlite_type("CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("VARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("VARYING CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NATIVE CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        assert(*to_sqlite_type("NVARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
    }
    
    //  blob..
    assert(*to_sqlite_type("BLOB") == sqlite_type::BLOB);
    
    //  real
    assert(*to_sqlite_type("REAL") == sqlite_type::REAL);
    assert(*to_sqlite_type("DOUBLE") == sqlite_type::REAL);
    assert(*to_sqlite_type("DOUBLE PRECISION") == sqlite_type::REAL);
    assert(*to_sqlite_type("FLOAT") == sqlite_type::REAL);
    
    assert(*to_sqlite_type("NUMERIC") == sqlite_type::REAL);
    for(auto i = 0; i < 255; ++i) {
        for(auto j = 0; j < 10; ++j) {
            assert(*to_sqlite_type("DECIMAL(" + std::to_string(i) + "," + std::to_string(j) + ")") == sqlite_type::REAL);
        }
    }
    assert(*to_sqlite_type("BOOLEAN") == sqlite_type::REAL);
    assert(*to_sqlite_type("DATE") == sqlite_type::REAL);
    assert(*to_sqlite_type("DATETIME") == sqlite_type::REAL);
    
    
    
    assert(type_is_nullable<bool>::value == false);
    assert(type_is_nullable<char>::value == false);
    assert(type_is_nullable<unsigned char>::value == false);
    assert(type_is_nullable<signed char>::value == false);
    assert(type_is_nullable<short>::value == false);
    assert(type_is_nullable<unsigned short>::value == false);
    assert(type_is_nullable<int>::value == false);
    assert(type_is_nullable<unsigned int>::value == false);
    assert(type_is_nullable<long>::value == false);
    assert(type_is_nullable<unsigned long>::value == false);
    assert(type_is_nullable<long long>::value == false);
    assert(type_is_nullable<unsigned long long>::value == false);
    assert(type_is_nullable<float>::value == false);
    assert(type_is_nullable<double>::value == false);
    assert(type_is_nullable<long double>::value == false);
    assert(type_is_nullable<long double>::value == false);
    assert(type_is_nullable<std::string>::value == false);
    assert(type_is_nullable<std::shared_ptr<int>>::value == true);
    assert(type_is_nullable<std::shared_ptr<std::string>>::value == true);
    assert(type_is_nullable<std::unique_ptr<int>>::value == true);
    assert(type_is_nullable<std::unique_ptr<std::string>>::value == true);

}

/**
 *  this is the deal: assume we have a `users` table with schema
 *  `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL, category_id INTEGER, surname TEXT)`.
 *  We create a storage and insert several objects. Next we simulate schema changing (app update for example): create
 *  another storage with a new schema partial of previous one: `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL)`.
 *  Next we call `sync_schema(true)` and assert that all users are saved. This test tests whether REMOVE COLUMN imitation works well.
 */
void testSyncSchema() {
    
    //  this is an old version of user..
    struct UserBefore {
        int id;
        std::string name;
        std::shared_ptr<int> categoryId;
        std::shared_ptr<std::string> surname;
    };
    
    //  this is an new version of user..
    struct UserAfter {
        int id;
        std::string name;
    };
    
    //  create an old storage..
    auto filename = "sync_schema_text.sqlite";
    auto storage = make_storage(filename,
                                make_table("users",
                                           make_column("id",
                                                       &UserBefore::id,
                                                       primary_key()),
                                           make_column("name",
                                                       &UserBefore::name),
                                           make_column("category_id",
                                                       &UserBefore::categoryId),
                                           make_column("surname",
                                                       &UserBefore::surname)));
    
    //  sync in case if it is first launch..
    storage.sync_schema();
    
    //  remove old users in case the test was launched before..
    storage.remove_all<UserBefore>();
    
    //  create c++ objects to insert into table..
    std::vector<UserBefore> usersToInsert {
        { -1, "Michael", nullptr, std::make_shared<std::string>("Scofield") },
        { -1, "Lincoln", std::make_shared<int>(4), std::make_shared<std::string>("Burrows") },
        { -1, "Sucre", nullptr, nullptr },
        { -1, "Sara", std::make_shared<int>(996), std::make_shared<std::string>("Tancredi") },
        { -1, "John", std::make_shared<int>(100500), std::make_shared<std::string>("Abruzzi") },
        { -1, "Brad", std::make_shared<int>(65), nullptr },
        { -1, "Paul", std::make_shared<int>(65), nullptr },
    };
    
    for(auto &user : usersToInsert) {
        auto insertedId = storage.insert(user);
        user.id = insertedId;
    }
    
    //  assert count first cause we will be asserting row by row next..
    assert(storage.count<UserBefore>() == usersToInsert.size());
    
    //  now we create new storage with partial schema..
    auto newStorage = make_storage(filename,
                                   make_table("users",
                                              make_column("id",
                                                          &UserAfter::id,
                                                          primary_key()),
                                              make_column("name",
                                                          &UserAfter::name)));
    
    //  now call `sync_schema` with argument `preserve` as `true`. It will cause retain data in case `sqlite_orm` needs to remove column..
    newStorage.sync_schema(true);
    
    //  get all users after syncing schema..
    auto usersFromDb = newStorage.get_all<UserAfter>(order_by(&UserAfter::id));
    
    assert(usersFromDb.size() == usersToInsert.size());
    
    for(auto i = 0; i < usersFromDb.size(); ++i) {
        auto &userFromDb = usersFromDb[i];
        auto &oldUser = usersToInsert[i];
        assert(userFromDb.id == oldUser.id);
        assert(userFromDb.name == oldUser.name);
    }
    
    auto usersCountBefore = newStorage.count<UserAfter>();
    
    newStorage.sync_schema();
    
    auto usersCountAfter = newStorage.count<UserAfter>();
    assert(usersCountBefore == usersCountAfter);
    
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
    assert(std::equal(ids.begin(),
                      ids.end(),
                      idsFromGetAll.begin(),
                      idsFromGetAll.end()));
    
}

int main() {
    
    testTypeParsing();
    
    testSyncSchema();
}
