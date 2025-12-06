#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>
#include <cstring>  //  std::strcmp
#include "catch_matchers.h"

using namespace sqlite_orm;

TEST_CASE("explicit from") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.replace(User{1, "Bebe Rexha"});

    std::vector<decltype(User::id)> rows;
    decltype(rows) expected;
    SECTION("without conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>());
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id), from<als>());
        }
        expected.push_back(1);
    }
    SECTION("with real conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>(), where(is_equal(&User::name, "Bebe Rexha")));
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id),
                                  from<als>(),
                                  where(is_equal(alias_column<als>(&User::name), "Bebe Rexha")));
        }
        expected.push_back(1);
    }
    SECTION("with unreal conditions") {
        SECTION("without alias") {
            rows = storage.select(&User::id, from<User>(), where(is_equal(&User::name, "Zara Larsson")));
        }
        SECTION("with alias") {
            using als = alias_u<User>;
            rows = storage.select(alias_column<als>(&User::id),
                                  from<als>(),
                                  where(is_equal(alias_column<als>(&User::name), "Zara Larsson")));
        }
    }
    REQUIRE(expected == rows);
}

TEST_CASE("update_all") {
    struct Record {
        int id = 0;
        std::string name;
    };
    auto storage = make_storage(
        {},
        make_table("records", make_column("id", &Record::id, primary_key()), make_column("name", &Record::name)));
    storage.sync_schema();
    auto vars = dynamic_set(storage);
    vars.push_back(assign(&Record::name, "Bob"));
    storage.update_all(vars, where(is_equal(&Record::id, 10)));
}

TEST_CASE("update set null") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.replace(User{1, std::make_unique<std::string>("Ototo")});
    REQUIRE(storage.count<User>() == 1);
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
    }

    storage.update_all(set(assign(&User::name, nullptr)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE_FALSE(rows.front().name);
    }

    storage.update_all(set(assign(&User::name, "ototo")));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
        REQUIRE(*rows.front().name == "ototo");
    }

    storage.update_all(set(assign(&User::name, nullptr)), where(is_equal(&User::id, 1)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE_FALSE(rows.front().name);
    }
}

namespace {
    struct CustomRowIdKeyType {
        std::uint32_t low;
        std::int32_t high;
    };
}
template<>
struct sqlite_orm::type_printer<CustomRowIdKeyType> : public integer_printer {};

TEST_CASE("InsertRange") {
    struct Object {
        int64 id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        Object() = default;
        Object(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };
    struct Object2 {
        int64 id = 0;
        std::string name;
    };
    struct Object3 {
        int objectId = 0;
        int discriminatingId = 0;
    };
    struct Object4 {
        int objectId = 0;
        int discriminatingId = 0;
    };
    struct Object5 {
        CustomRowIdKeyType id = {};
        std::string name;
    };
#if SQLITE_VERSION_NUMBER >= 3008002
    struct ObjectWithoutRowid {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        ObjectWithoutRowid() = default;
        ObjectWithoutRowid(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };
    struct ObjectWithoutRowid2 {
        int id = 0;
    };
    struct ObjectWithoutRowid3 {
        int objectId = 0;
        int discriminatingId = 0;
    };
    struct ObjectWithoutRowid4 {
        int objectId = 0;
        int discriminatingId = 0;
    };
#endif

    auto storage = make_storage(
        "test_insert_range.sqlite",
        // with rowid, column pk
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
        // with rowid, single table pk
        make_table("objects2",
                   make_column("id", &Object2::id),
                   make_column("name", &Object2::name),
                   primary_key(&Object2::id)),
        // with rowid, composite table pk
        make_table("objects3",
                   make_column("object_id", &Object3::objectId),
                   make_column("discriminating_id", &Object3::discriminatingId),
                   primary_key(&Object3::objectId, &Object3::discriminatingId)),
        // with rowid, composite table pk involving a default value
        make_table("objects4",
                   make_column("object_id", &Object4::objectId),
                   make_column("discriminating_id", &Object4::discriminatingId, default_value(1)),
                   primary_key(&Object4::objectId, &Object4::discriminatingId)),
        // with rowid, column pk of custom type
        make_table("objects5", make_column("id", &Object5::id, primary_key()), make_column("name", &Object5::name))
#if SQLITE_VERSION_NUMBER >= 3008002
            ,
        // without rowid, column pk
        make_table("objects_without_rowid",
                   make_column("id", &ObjectWithoutRowid::id, primary_key()),
                   make_column("name", &ObjectWithoutRowid::name))
            .without_rowid(),
        // without rowid, single table pk
        make_table("objects_without_rowid2",
                   make_column("id", &ObjectWithoutRowid2::id),
                   primary_key(&ObjectWithoutRowid2::id))
            .without_rowid(),
        // with rowid, composite table pk
        make_table("objects_without_rowid3",
                   make_column("object_id", &ObjectWithoutRowid3::objectId),
                   make_column("discriminating_id", &ObjectWithoutRowid3::discriminatingId),
                   primary_key(&ObjectWithoutRowid3::objectId, &ObjectWithoutRowid3::discriminatingId))
            .without_rowid(),
        // with rowid, composite table pk involving a default value
        make_table("objects_without_rowid4",
                   make_column("object_id", &ObjectWithoutRowid4::objectId),
                   make_column("discriminating_id", &ObjectWithoutRowid4::discriminatingId, default_value(1)),
                   primary_key(&ObjectWithoutRowid4::objectId, &ObjectWithoutRowid4::discriminatingId))
            .without_rowid()
#endif
    );

    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<Object2>();
    storage.remove_all<Object3>();
    storage.remove_all<Object4>();
    storage.remove_all<Object5>();
#if SQLITE_VERSION_NUMBER >= 3008002
    storage.remove_all<ObjectWithoutRowid>();
    storage.remove_all<ObjectWithoutRowid2>();
    storage.remove_all<ObjectWithoutRowid3>();
    storage.remove_all<ObjectWithoutRowid4>();
#endif

    SECTION("straight") {
        std::vector<Object> objects = {100,
                                       Object{
                                           0,
                                           "Skillet",
                                       }};
        storage.insert_range(objects.begin(), objects.end());
        REQUIRE(storage.count<Object>() == 100);

        // empty container
        std::vector<Object> emptyVector;
        REQUIRE_NOTHROW(storage.insert_range(emptyVector.begin(), emptyVector.end()));

        // with rowid, single table pk
        std::vector<Object2> objects2 = {{2}, {4}};
        storage.insert_range(objects2.begin(), objects2.end());
        REQUIRE_NOTHROW(storage.get<Object2>(1));

        // with rowid, composite table pk
        std::vector<Object3> objects3 = {{2, 2}, {4, 4}};
        storage.insert_range(objects3.begin(), objects3.end());
        REQUIRE_NOTHROW(storage.get<Object3>(2, 2));

        // with rowid, composite table pk involving a default value
        std::vector<Object4> objects4 = {{2, 2}, {4, 4}};
        storage.insert_range(objects4.begin(), objects4.end());
        REQUIRE_NOTHROW(storage.get<Object4>(2, 1));

        // with rowid, single table pk
        std::vector<Object5> objects5 = {{}, {}};
        storage.insert_range(objects5.begin(), objects5.end());
        REQUIRE(storage.count<Object5>(where(c(&Object5::id) == 1)) == 1);

#if SQLITE_VERSION_NUMBER >= 3008002
        // without rowid, column pk
        std::vector<ObjectWithoutRowid> objectsWR1 = {{10, "Life"}, {20, "Death"}};
        storage.insert_range(objectsWR1.begin(), objectsWR1.end());
        REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
        REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");

        // without rowid, single table pk
        std::vector<ObjectWithoutRowid2> objectsWR2 = {{2}};
        storage.insert_range(objectsWR2.begin(), objectsWR2.end());
        REQUIRE_NOTHROW(storage.get<ObjectWithoutRowid2>(2));

        // without rowid, composite table pk
        std::vector<ObjectWithoutRowid3> objectsWR3 = {{2, 2}, {4, 4}};
        storage.insert_range(objectsWR3.begin(), objectsWR3.end());
        REQUIRE_NOTHROW(storage.get<ObjectWithoutRowid3>(2, 2));

        // without rowid, composite table pk involving a default value
        std::vector<ObjectWithoutRowid4> objectsWR4 = {{2, 2}, {4, 4}};
        storage.insert_range(objectsWR4.begin(), objectsWR4.end());
        REQUIRE_NOTHROW(storage.get<ObjectWithoutRowid4>(2, 1));
#endif
    }
    SECTION("pointers") {
        std::vector<std::unique_ptr<Object>> objects;
        objects.reserve(100);
        for (auto i = 0; i < 100; ++i) {
            objects.push_back(std::make_unique<Object>(0, "Skillet"));
        }
        storage.insert_range(objects.begin(), objects.end(), &std::unique_ptr<Object>::operator*);
        REQUIRE(storage.count<Object>() == 100);

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        REQUIRE_NOTHROW(
            storage.insert_range(emptyVector.begin(), emptyVector.end(), &std::unique_ptr<Object>::operator*));

#if SQLITE_VERSION_NUMBER >= 3008002
        //  test insert_range without rowid
        std::vector<std::unique_ptr<ObjectWithoutRowid>> objectsWR;
        objectsWR.push_back(std::make_unique<ObjectWithoutRowid>(10, "Life"));
        objectsWR.push_back(std::make_unique<ObjectWithoutRowid>(20, "Death"));

        REQUIRE(objectsWR.size() == 2);
        storage.insert_range(objectsWR.begin(), objectsWR.end(), &std::unique_ptr<ObjectWithoutRowid>::operator*);
        REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
        REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
#endif
    }
}

TEST_CASE("Select") {
    sqlite3* db;
    auto dbFileName = "test.db";
    auto rc = sqlite3_open(dbFileName, &db);
    REQUIRE(rc == SQLITE_OK);
    auto sql = "CREATE TABLE IF NOT EXISTS WORDS("
               "ID INTEGER PRIMARY        KEY AUTOINCREMENT      NOT NULL,"
               "CURRENT_WORD          TEXT     NOT NULL,"
               "BEFORE_WORD           TEXT     NOT NULL,"
               "AFTER_WORD            TEXT     NOT NULL,"
               "OCCURANCES            INT      NOT NULL);";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    REQUIRE(rc == SQLITE_OK);

    sqlite3_stmt* stmt;

    //  delete previous words. This command is excess in travis or other docker based CI tools
    //  but it is required on local machine
    sql = "DELETE FROM WORDS";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    REQUIRE(rc == SQLITE_OK);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
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
    if (rc != SQLITE_DONE) {
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
    if (rc != SQLITE_DONE) {
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
        if (rc != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        REQUIRE(sqlite3_column_int(stmt, 0) == firstId);
        REQUIRE(std::strcmp((const char*)sqlite3_column_text(stmt, 1), "best") == 0);
        REQUIRE(std::strcmp((const char*)sqlite3_column_text(stmt, 2), "behaviour") == 0);
        REQUIRE(std::strcmp((const char*)sqlite3_column_text(stmt, 3), "hey") == 0);
        REQUIRE(sqlite3_column_int(stmt, 4) == 5);
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);

    struct Word {
        int64 id;
        std::string currentWord;
        std::string beforeWord;
        std::string afterWord;
        int occurances;
    };

    auto storage = make_storage(dbFileName,
                                make_table("WORDS",
                                           make_column("ID", &Word::id, primary_key().autoincrement()),
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

    auto cols = columns(&Word::id, &Word::currentWord, &Word::beforeWord, &Word::afterWord, &Word::occurances);
    auto rawTuples = storage.select(cols, where(eq(&Word::id, firstId)));
    REQUIRE(rawTuples.size() == 1);

    {
        auto& firstTuple = rawTuples.front();
        REQUIRE(std::get<0>(firstTuple) == firstId);
        REQUIRE(std::get<1>(firstTuple) == "best");
        REQUIRE(std::get<2>(firstTuple) == "behaviour");
        REQUIRE(std::get<3>(firstTuple) == "hey");
        REQUIRE(std::get<4>(firstTuple) == 5);
    }

    rawTuples = storage.select(cols, where(eq(&Word::id, secondId)));
    REQUIRE(rawTuples.size() == 1);

    {
        auto& secondTuple = rawTuples.front();
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

    storage.update_all(set(assign(&Word::currentWord, "ototo")), where(is_equal(&Word::id, firstId)));

    REQUIRE(storage.get<Word>(firstId).currentWord == "ototo");

    std::remove(dbFileName);
}

TEST_CASE("Replace query") {
    struct Object {
        int id = 0;
        std::string name;

#ifndef SQLITE_ORM_AGGREGATE_PAREN_INIT_SUPPORTED
        Object() = default;
        Object(int id, std::string name) : id{id}, name{std::move(name)} {}
#endif
    };

    struct User {

        User(int id_, std::string name_) : id(id_), name(std::move(name_)) {}

        int getId() const {
            return this->id;
        }

        void setId(int id_) {
            this->id = id_;
        }

        std::string getName() const {
            return this->name;
        }

        void setName(std::string name_) {
            this->name = std::move(name_);
        }

      private:
        int id = 0;
        std::string name;
    };

    auto storage = make_storage(
        "test_replace.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
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

    SECTION("straight") {
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
        storage.replace_range(emptyVector.begin(), emptyVector.end());
    }
    SECTION("pointers") {
        std::vector<std::unique_ptr<Object>> vector;
        vector.push_back(std::make_unique<Object>(300, "Iggy"));
        vector.push_back(std::make_unique<Object>(400, "Azalea"));
        storage.replace_range(vector.begin(), vector.end(), &std::unique_ptr<Object>::operator*);
        REQUIRE(storage.count<Object>() == 4);

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        storage.replace_range(emptyVector.begin(), emptyVector.end(), &std::unique_ptr<Object>::operator*);
    }
    REQUIRE(storage.count<User>() == 0);
    storage.replace(User{10, "Daddy Yankee"});
}

TEST_CASE("Remove all") {
    struct Object {
        int id = 0;
        std::string name;
    };

    auto storage = make_storage(
        "",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));
    storage.sync_schema();

    storage.replace(Object{1, "Ototo"});
    storage.replace(Object{2, "Contigo"});

    REQUIRE(storage.count<Object>() == 2);

    storage.remove_all<Object>(where(c(&Object::id) == 1));

    REQUIRE(storage.count<Object>() == 1);
}

TEST_CASE("Explicit insert") {
#if SQLITE_VERSION_NUMBER >= 3037002
    const ErrorCodeExceptionMatcher notNullExceptionMatcher(sqlite_errc(SQLITE_CONSTRAINT_NOTNULL));
#else
    const ErrorCodeExceptionMatcher notNullExceptionMatcher(sqlite_errc(SQLITE_CONSTRAINT));
#endif

    struct User {
        int id;
        std::string name;
        int age;
        std::string email;
    };

    class Visit {
      public:
        const int& id() const {
            return _id;
        }

        void setId(int newValue) {
            _id = newValue;
        }

        const time_t& createdAt() const {
            return _createdAt;
        }

        void setCreatedAt(time_t newValue) {
            _createdAt = newValue;
        }

        const int& usedId() const {
            return _usedId;
        }

        void setUsedId(int newValue) {
            _usedId = newValue;
        }

      private:
        int _id;
        time_t _createdAt;
        int _usedId;
    };

    auto storage =
        make_storage("explicitinsert.sqlite",
                     make_table("users",
                                make_column("id", &User::id, primary_key()),
                                make_column("name", &User::name),
                                make_column("age", &User::age),
                                make_column("email", &User::email, default_value("dummy@email.com"))),
                     make_table("visits",
                                make_column("id", &Visit::setId, &Visit::id, primary_key()),
                                make_column("created_at", &Visit::createdAt, &Visit::setCreatedAt, default_value(10)),
                                make_column("used_id", &Visit::usedId, &Visit::setUsedId)));

    storage.sync_schema();
    storage.remove_all<User>();
    storage.remove_all<Visit>();

    SECTION("user") {
        SECTION("two columns") {
            User user{};
            user.name = "Juan";
            user.age = 57;
            auto id = storage.insert(user, columns(&User::name, &User::age));
            REQUIRE(storage.get<User>(id).email == "dummy@email.com");
        }
        SECTION("three columns") {
            User user2;
            user2.id = 2;
            user2.name = "Kevin";
            user2.age = 27;
            REQUIRE(user2.id == storage.insert(user2, columns(&User::id, &User::name, &User::age)));
            REQUIRE(storage.get<User>(user2.id).email == "dummy@email.com");
        }
        SECTION("four columns") {
            User user3;
            user3.id = 3;
            user3.name = "Sia";
            user3.age = 42;
            user3.email = "sia@gmail.com";
            auto insertedId = storage.insert(user3, columns(&User::id, &User::name, &User::age, &User::email));
            REQUIRE(user3.id == insertedId);
            auto insertedUser3 = storage.get<User>(user3.id);
            REQUIRE(insertedUser3.email == user3.email);
            REQUIRE(insertedUser3.age == user3.age);
            REQUIRE(insertedUser3.name == user3.name);
        }
        SECTION("one column") {
            User user4;
            user4.name = "Egor";
            REQUIRE_THROWS_MATCHES(storage.insert(user4, columns(&User::name)),
                                   std::system_error,
                                   notNullExceptionMatcher);
        }
    }
    SECTION("visit") {
        SECTION("one column not primary key") {
            Visit visit;
            SECTION("getter") {
                visit.setUsedId(1);
                visit.setId(storage.insert(visit, columns(&Visit::usedId)));

                auto visitFromStorage = storage.get<Visit>(visit.id());
                REQUIRE(visitFromStorage.createdAt() == 10);
                REQUIRE(visitFromStorage.usedId() == visit.usedId());
                storage.remove<Visit>(visitFromStorage.usedId());
            }
        }
        SECTION("two columns") {
            Visit visit2;
            visit2.setId(2);
            visit2.setUsedId(1);
            SECTION("getters") {
                auto insertedId = storage.insert(visit2, columns(&Visit::id, &Visit::usedId));
                REQUIRE(visit2.id() == insertedId);
                auto visitFromStorage = storage.get<Visit>(visit2.id());
                REQUIRE(visitFromStorage.usedId() == visit2.usedId());
                storage.remove<Visit>(visit2.id());
            }
        }
        SECTION("one column primary key") {
            Visit visit3;
            visit3.setId(10);
            SECTION("getter") {
                REQUIRE_THROWS_MATCHES(storage.insert(visit3, columns(&Visit::id)),
                                       std::system_error,
                                       notNullExceptionMatcher);
            }
        }
    }
}
