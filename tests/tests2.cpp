#include <sqlite_orm/sqlite_orm.h>
#include <iostream>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Empty storage"){
    auto storage = make_storage("empty.sqlite");
    storage.table_exists("table");
}

TEST_CASE("Remove"){
    struct Object {
        int id;
        std::string name;
    };
    
    {
        auto storage = make_storage("test_remove.sqlite",
                                    make_table("objects",
                                               make_column("id", &Object::id, primary_key()),
                                               make_column("name", &Object::name)));
        storage.sync_schema();
        storage.remove_all<Object>();
        
        auto id1 = storage.insert(Object{ 0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("test_remove.sqlite",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id)));
        storage.sync_schema();
        storage.remove_all<Object>();
        
        auto id1 = storage.insert(Object{ 0, "Skillet"});
        REQUIRE(storage.count<Object>() == 1);
        storage.remove<Object>(id1);
        REQUIRE(storage.count<Object>() == 0);
    }
    {
        auto storage = make_storage("",
                                    make_table("objects",
                                               make_column("id", &Object::id),
                                               make_column("name", &Object::name),
                                               primary_key(&Object::id, &Object::name)));
        storage.sync_schema();
        storage.replace(Object{1, "Skillet"});
        assert(storage.count<Object>() == 1);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 0);
        
        storage.replace(Object{1, "Skillet"});
        storage.replace(Object{2, "Paul Cless"});
        REQUIRE(storage.count<Object>() == 2);
        storage.remove<Object>(1, "Skillet");
        REQUIRE(storage.count<Object>() == 1);
        
    }
}

TEST_CASE("Select"){
    sqlite3 *db;
    auto dbFileName = "test.db";
    auto rc = sqlite3_open(dbFileName, &db);
    assert(rc == SQLITE_OK);
    auto sql = "CREATE TABLE IF NOT EXISTS WORDS("
    "ID INTEGER PRIMARY        KEY AUTOINCREMENT      NOT NULL,"
    "CURRENT_WORD          TEXT     NOT NULL,"
    "BEFORE_WORD           TEXT     NOT NULL,"
    "AFTER_WORD            TEXT     NOT NULL,"
    "OCCURANCES            INT      NOT NULL);";
    
    char *errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    REQUIRE(rc == SQLITE_OK);
    
    sqlite3_stmt *stmt;
    
    //  delete previous words. This command is excess in travis or other docker based CI tools
    //  but it is required on local machine
    sql = "DELETE FROM WORDS";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    
    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        cout << sqlite3_errmsg(db) << endl;
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    
    sql = "INSERT INTO WORDS (CURRENT_WORD, BEFORE_WORD, AFTER_WORD, OCCURANCES) VALUES(?, ?, ?, ?)";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    
    //  INSERT [ ID, 'best', 'behaviour', 'hey', 5 ]
    
    sqlite3_bind_text(stmt, 1, "best", -1, nullptr);
    sqlite3_bind_text(stmt, 2, "behaviour", -1, nullptr);
    sqlite3_bind_text(stmt, 3, "hey", -1, nullptr);
    sqlite3_bind_int(stmt, 4, 5);
    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        cout << sqlite3_errmsg(db) << endl;
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    
    auto firstId = sqlite3_last_insert_rowid(db);
    
    //  INSERT [ ID, 'corruption', 'blood', 'brothers', 15 ]
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);
    sqlite3_bind_text(stmt, 1, "corruption", -1, nullptr);
    sqlite3_bind_text(stmt, 2, "blood", -1, nullptr);
    sqlite3_bind_text(stmt, 3, "brothers", -1, nullptr);
    sqlite3_bind_int(stmt, 4, 15);
    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE){
        cout << sqlite3_errmsg(db) << endl;
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);
    
    auto secondId = sqlite3_last_insert_rowid(db);
    
    {
        //  SELECT ID, CURRENT_WORD, BEFORE_WORD, AFTER_WORD, OCCURANCES
        //  FROM WORDS
        //  WHERE ID = firstId
        
        sql = "SELECT ID, CURRENT_WORD, BEFORE_WORD, AFTER_WORD, OCCURANCES FROM WORDS WHERE ID = ?";
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        REQUIRE(rc == SQLITE_OK);
        
        sqlite3_bind_int64(stmt, 1, firstId);
        rc = sqlite3_step(stmt);
        if(rc != SQLITE_ROW){
            cout << sqlite3_errmsg(db) << endl;
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        REQUIRE(sqlite3_column_int(stmt, 0) == firstId);
        REQUIRE(::strcmp((const char *)sqlite3_column_text(stmt, 1), "best") == 0);
        REQUIRE(::strcmp((const char *)sqlite3_column_text(stmt, 2), "behaviour") == 0);
        REQUIRE(::strcmp((const char *)sqlite3_column_text(stmt, 3), "hey") == 0);
        REQUIRE(sqlite3_column_int(stmt, 4) == 5);
        sqlite3_finalize(stmt);
    }
    
    sqlite3_close(db);
    
    struct Word {
        int id;
        std::string currentWord;
        std::string beforeWord;
        std::string afterWord;
        int occurances;
    };
    
    auto storage = make_storage(dbFileName,
                                make_table("WORDS",
                                           make_column("ID", &Word::id, primary_key(), autoincrement()),
                                           make_column("CURRENT_WORD", &Word::currentWord),
                                           make_column("BEFORE_WORD", &Word::beforeWord),
                                           make_column("AFTER_WORD", &Word::afterWord),
                                           make_column("OCCURANCES", &Word::occurances)));
    
    storage.sync_schema();  //  sync schema must not alter any data cause schemas are the same
    
    REQUIRE(storage.count<Word>() == 2);
    
    auto firstRow = storage.get_no_throw<Word>(firstId);
    REQUIRE(firstRow);
    REQUIRE(firstRow->currentWord == "best");
    REQUIRE(firstRow->beforeWord == "behaviour");
    REQUIRE(firstRow->afterWord == "hey");
    REQUIRE(firstRow->occurances == 5);
    
    auto secondRow = storage.get_pointer<Word>(secondId);
    REQUIRE(secondRow);
    REQUIRE(secondRow->currentWord == "corruption");
    REQUIRE(secondRow->beforeWord == "blood");
    REQUIRE(secondRow->afterWord == "brothers");
    REQUIRE(secondRow->occurances == 15);
    
    auto cols = columns(&Word::id,
                        &Word::currentWord,
                        &Word::beforeWord,
                        &Word::afterWord,
                        &Word::occurances);
    auto rawTuples = storage.select(cols, where(eq(&Word::id, firstId)));
    REQUIRE(rawTuples.size() == 1);
    
    {
        auto &firstTuple = rawTuples.front();
        REQUIRE(std::get<0>(firstTuple) == firstId);
        REQUIRE(std::get<1>(firstTuple) == "best");
        REQUIRE(std::get<2>(firstTuple) == "behaviour");
        REQUIRE(std::get<3>(firstTuple) == "hey");
        REQUIRE(std::get<4>(firstTuple) == 5);
    }
    
    rawTuples = storage.select(cols, where(eq(&Word::id, secondId)));
    REQUIRE(rawTuples.size() == 1);
    
    {
        auto &secondTuple = rawTuples.front();
        REQUIRE(std::get<0>(secondTuple) == secondId);
        REQUIRE(std::get<1>(secondTuple) == "corruption");
        REQUIRE(std::get<2>(secondTuple) == "blood");
        REQUIRE(std::get<3>(secondTuple) == "brothers");
        REQUIRE(std::get<4>(secondTuple) == 15);
    }
    
    auto ordr = order_by(&Word::id);
    
    auto idsOnly = storage.select(&Word::id, ordr);
    REQUIRE(idsOnly.size() == 2);
    
    REQUIRE(idsOnly[0] == firstId);
    REQUIRE(idsOnly[1] == secondId);
    
    auto currentWordsOnly = storage.select(&Word::currentWord, ordr);
    REQUIRE(currentWordsOnly.size() == 2);
    
    REQUIRE(currentWordsOnly[0] == "best");
    REQUIRE(currentWordsOnly[1] == "corruption");
    
    auto beforeWordsOnly = storage.select(&Word::beforeWord, ordr);
    REQUIRE(beforeWordsOnly.size() == 2);
    
    REQUIRE(beforeWordsOnly[0] == "behaviour");
    REQUIRE(beforeWordsOnly[1] == "blood");
    
    auto afterWordsOnly = storage.select(&Word::afterWord, ordr);
    REQUIRE(afterWordsOnly.size() == 2);
    
    REQUIRE(afterWordsOnly[0] == "hey");
    REQUIRE(afterWordsOnly[1] == "brothers");
    
    auto occurencesOnly = storage.select(&Word::occurances, ordr);
    REQUIRE(occurencesOnly.size() == 2);
    
    REQUIRE(occurencesOnly[0] == 5);
    REQUIRE(occurencesOnly[1] == 15);
    
    //  test update_all with the same storage
    
    storage.update_all(set(assign(&Word::currentWord, "ototo")),
                       where(is_equal(&Word::id, firstId)));
    
    REQUIRE(storage.get<Word>(firstId).currentWord == "ototo");
    
}

TEST_CASE("Replace"){
    struct Object {
        int id;
        std::string name;
    };
    
    struct User {
        
        User(int id_, std::string name_): id(id_), name(move(name_)) {}
        
        int getId() const {
            return this->id;
        }
        
        void setId(int id) {
            this->id = id;
        }
        
        std::string getName() const {
            return this->name;
        }
        
        void setName(std::string name) {
            this->name = move(name);
        }
        
    private:
        int id = 0;
        std::string name;
    };
    
    auto storage = make_storage("test_replace.sqlite",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key()),
                                           make_column("name", &Object::name)),
                                make_table("users",
                                           make_column("id", &User::getId, &User::setId, primary_key()),
                                           make_column("name", &User::setName, &User::getName)));
    
    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<User>();
    
    storage.replace(Object{
        100,
        "Baby",
    });
    REQUIRE(storage.count<Object>() == 1);
    auto baby = storage.get<Object>(100);
    REQUIRE(baby.id == 100);
    REQUIRE(baby.name == "Baby");
    
    storage.replace(Object{
        200,
        "Time",
    });
    REQUIRE(storage.count<Object>() == 2);
    auto time = storage.get<Object>(200);
    REQUIRE(time.id == 200);
    REQUIRE(time.name == "Time");
    storage.replace(Object{
        100,
        "Ototo",
    });
    REQUIRE(storage.count<Object>() == 2);
    auto ototo = storage.get<Object>(100);
    REQUIRE(ototo.id == 100);
    REQUIRE(ototo.name == "Ototo");
    
    auto initList = {
        Object{
            300,
            "Iggy",
        },
        Object{
            400,
            "Azalea",
        },
    };
    storage.replace_range(initList.begin(), initList.end());
    REQUIRE(storage.count<Object>() == 4);
    
    //  test empty container
    std::vector<Object> emptyVector;
    storage.replace_range(emptyVector.begin(),
                          emptyVector.end());
    
    
    REQUIRE(storage.count<User>() == 0);
    storage.replace(User{10, "Daddy Yankee"});
}

TEST_CASE("Insert"){
    struct Object {
        int id;
        std::string name;
    };
    
    struct ObjectWithoutRowid {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("test_insert.sqlite",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key()),
                                           make_column("name", &Object::name)),
                                make_table("objects_without_rowid",
                                           make_column("id", &ObjectWithoutRowid::id, primary_key()),
                                           make_column("name", &ObjectWithoutRowid::name)).without_rowid());
    
    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<ObjectWithoutRowid>();
    
    for(auto i = 0; i < 100; ++i) {
        storage.insert(Object{
            0,
            "Skillet",
        });
        REQUIRE(storage.count<Object>() == i + 1);
    }
    
    auto initList = {
        Object{
            0,
            "Insane",
        },
        Object{
            0,
            "Super",
        },
        Object{
            0,
            "Sun",
        },
    };
    
    cout << "inserting range" << endl;
    auto countBefore = storage.count<Object>();
    storage.insert_range(initList.begin(),
                         initList.end());
    REQUIRE(storage.count<Object>() == countBefore + static_cast<int>(initList.size()));
    
    
    //  test empty container
    std::vector<Object> emptyVector;
    storage.insert_range(emptyVector.begin(),
                         emptyVector.end());
    
    //  test insert without rowid
    storage.insert(ObjectWithoutRowid{ 10, "Life" });
    REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
    storage.insert(ObjectWithoutRowid{ 20, "Death" });
    REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
}

TEST_CASE("Type parsing"){
    using namespace sqlite_orm::internal;
    
    //  int
    REQUIRE(*to_sqlite_type("INT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("integeer") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INTEGER") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("TINYINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("SMALLINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("MEDIUMINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("BIGINT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("UNSIGNED BIG INT") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INT2") == sqlite_type::INTEGER);
    REQUIRE(*to_sqlite_type("INT8") == sqlite_type::INTEGER);
    
    //  text
    REQUIRE(*to_sqlite_type("TEXT") == sqlite_type::TEXT);
    REQUIRE(*to_sqlite_type("CLOB") == sqlite_type::TEXT);
    for(auto i = 0; i< 255; ++i) {
        REQUIRE(*to_sqlite_type("CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("VARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("VARYING CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NATIVE CHARACTER(" + std::to_string(i) + ")") == sqlite_type::TEXT);
        REQUIRE(*to_sqlite_type("NVARCHAR(" + std::to_string(i) + ")") == sqlite_type::TEXT);
    }
    
    //  blob..
    REQUIRE(*to_sqlite_type("BLOB") == sqlite_type::BLOB);
    
    //  real
    REQUIRE(*to_sqlite_type("REAL") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DOUBLE") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DOUBLE PRECISION") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("FLOAT") == sqlite_type::REAL);
    
    REQUIRE(*to_sqlite_type("NUMERIC") == sqlite_type::REAL);
    for(auto i = 0; i < 255; ++i) {
        for(auto j = 0; j < 10; ++j) {
            REQUIRE(*to_sqlite_type("DECIMAL(" + std::to_string(i) + "," + std::to_string(j) + ")") == sqlite_type::REAL);
        }
    }
    REQUIRE(*to_sqlite_type("BOOLEAN") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DATE") == sqlite_type::REAL);
    REQUIRE(*to_sqlite_type("DATETIME") == sqlite_type::REAL);
    
    
    
    REQUIRE(type_is_nullable<bool>::value == false);
    REQUIRE(type_is_nullable<char>::value == false);
    REQUIRE(type_is_nullable<unsigned char>::value == false);
    REQUIRE(type_is_nullable<signed char>::value == false);
    REQUIRE(type_is_nullable<short>::value == false);
    REQUIRE(type_is_nullable<unsigned short>::value == false);
    REQUIRE(type_is_nullable<int>::value == false);
    REQUIRE(type_is_nullable<unsigned int>::value == false);
    REQUIRE(type_is_nullable<long>::value == false);
    REQUIRE(type_is_nullable<unsigned long>::value == false);
    REQUIRE(type_is_nullable<long long>::value == false);
    REQUIRE(type_is_nullable<unsigned long long>::value == false);
    REQUIRE(type_is_nullable<float>::value == false);
    REQUIRE(type_is_nullable<double>::value == false);
    REQUIRE(type_is_nullable<long double>::value == false);
    REQUIRE(type_is_nullable<long double>::value == false);
    REQUIRE(type_is_nullable<std::string>::value == false);
    REQUIRE(type_is_nullable<std::unique_ptr<int>>::value == true);
    REQUIRE(type_is_nullable<std::unique_ptr<std::string>>::value == true);
    REQUIRE(type_is_nullable<std::unique_ptr<int>>::value == true);
    REQUIRE(type_is_nullable<std::unique_ptr<std::string>>::value == true);
    
}

/**
 *  this is the deal: assume we have a `users` table with schema
 *  `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL, category_id INTEGER, surname TEXT)`.
 *  We create a storage and insert several objects. Next we simulate schema changing (app update for example): create
 *  another storage with a new schema partial of previous one: `CREATE TABLE users (id INTEGER NOT NULL PRIMARY KEY, name TEXT NOT NULL)`.
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
    
    //  this is an new version of user..
    struct UserAfter {
        int id;
        std::string name;
    };
    
    //  create an old storage..
    auto filename = "sync_schema_text.sqlite";
    auto storage = make_storage(filename,
                                make_table("users",
                                           make_column("id", &UserBefore::id, primary_key()),
                                           make_column("name", &UserBefore::name),
                                           make_column("category_id", &UserBefore::categoryId),
                                           make_column("surname", &UserBefore::surname)));
    
    //  sync in case if it is first launch..
    auto syncSchemaSimulationRes = storage.sync_schema_simulate();
    auto syncSchemaRes = storage.sync_schema();
    
    REQUIRE(syncSchemaRes == syncSchemaSimulationRes);
    
    //  remove old users in case the test was launched before..
    storage.remove_all<UserBefore>();
    
    //  create c++ objects to insert into table..
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
    
    //  assert count first cause we will be asserting row by row next..
    REQUIRE(static_cast<size_t>(storage.count<UserBefore>()) == usersToInsert.size());
    
    //  now we create new storage with partial schema..
    auto newStorage = make_storage(filename,
                                   make_table("users",
                                              make_column("id",
                                                          &UserAfter::id,
                                                          primary_key()),
                                              make_column("name",
                                                          &UserAfter::name)));
    
    syncSchemaSimulationRes = newStorage.sync_schema_simulate(true);
    
    //  now call `sync_schema` with argument `preserve` as `true`. It will retain data in case `sqlite_orm` needs to remove a column..
    syncSchemaRes = newStorage.sync_schema(true);
    REQUIRE(syncSchemaRes.size() == 1);
    REQUIRE(syncSchemaRes.begin()->second == sync_schema_result::old_columns_removed);
    REQUIRE(syncSchemaSimulationRes == syncSchemaRes);
    
    //  get all users after syncing schema..
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
