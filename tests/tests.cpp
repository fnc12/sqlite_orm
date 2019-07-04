#include <sqlite_orm/sqlite_orm.h>

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <cassert>  //  assert
#include <vector>   //  std::vector
#include <string>   //  std::string
#include <iostream> //  std::cout, std::endl
#include <memory>   //  std::unique_ptr
#include <cstdio>   //  remove
#include <numeric>  //  std::iota
#include <algorithm>    //  std::fill


using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Case", "select"){
    
    struct User {
        int id = 0;
        std::string firstName;
        std::string lastName;
        std::string country;
    };
    
    struct Track {
        int id = 0;
        std::string name;
        long milliseconds = 0;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, autoincrement(), primary_key()),
                                           make_column("first_name", &User::firstName),
                                           make_column("last_name", &User::lastName),
                                           make_column("country", &User::country)),
                                make_table("tracks",
                                           make_column("trackid", &Track::id, autoincrement(), primary_key()),
                                           make_column("name", &Track::name),
                                           make_column("milliseconds", &Track::milliseconds)));
    storage.sync_schema();
    
    struct GradeAlias : alias_tag {
        static const std::string &get() {
            static const std::string res = "Grade";
            return res;
        }
    };
    
    {
        storage.insert(User{0, "Roberto", "Almeida", "Mexico"});
        storage.insert(User{0, "Julia", "Bernett", "USA"});
        storage.insert(User{0, "Camille", "Bernard", "Argentina"});
        storage.insert(User{0, "Michelle", "Brooks", "USA"});
        storage.insert(User{0, "Robet", "Brown", "USA"});
        
        auto rows = storage.select(columns(case_<std::string>(&User::country)
                                           .when("USA", then("Dosmetic"))
                                           .else_("Foreign")
                                           .end()),
                                   multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));
        auto verifyRows = [&storage](auto &rows){
            REQUIRE(rows.size() == storage.count<User>());
            REQUIRE(std::get<0>(rows[0]) == "Foreign");
            REQUIRE(std::get<0>(rows[1]) == "Foreign");
            REQUIRE(std::get<0>(rows[2]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[3]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[4]) == "Dosmetic");
        };
        verifyRows(rows);
        
        rows = storage.select(columns(as<GradeAlias>(case_<std::string>(&User::country)
                                                     .when("USA", then("Dosmetic"))
                                                     .else_("Foreign")
                                                     .end())),
                              multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));
        
        verifyRows(rows);
    }
    {
        storage.insert(Track{0, "For Those About To Rock", 400000});
        storage.insert(Track{0, "Balls to the Wall", 500000});
        storage.insert(Track{0, "Fast as a Shark", 200000});
        storage.insert(Track{0, "Restless and Wild", 100000});
        storage.insert(Track{0, "Princess of the Dawn", 50000});
        
        auto rows = storage.select(case_<std::string>()
                                   .when(c(&Track::milliseconds) < 60000, then("short"))
                                   .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                                   .else_("long")
                                   .end(),
                                   order_by(&Track::name));
        auto verifyRows = [&storage](auto &rows){
            REQUIRE(rows.size() == storage.count<Track>());
            REQUIRE(rows[0] == "long");
            REQUIRE(rows[1] == "medium");
            REQUIRE(rows[2] == "long");
            REQUIRE(rows[3] == "short");
            REQUIRE(rows[4] == "medium");
        };
        verifyRows(rows);
        
        rows = storage.select(as<GradeAlias>(case_<std::string>()
                                             .when(c(&Track::milliseconds) < 60000, then("short"))
                                             .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                                             .else_("long")
                                             .end()),
                              order_by(&Track::name));
        verifyRows(rows);
    }
    
    
}

TEST_CASE("Unique ptr in update", "update"){
    
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});
    
    {
        storage.update_all(set(assign(&User::name, std::make_unique<std::string>("Nick"))));
        REQUIRE(storage.count<User>(where(is_null(&User::name))) == 0);
    }
    {
        std::unique_ptr<std::string> ptr;
        storage.update_all(set(assign(&User::name, move(ptr))));
        REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 0);
    }
}

TEST_CASE("Join", "get_all"){
    
    struct User {
        int id = 0;
        std::string name;
    };
    
    struct Visit {
        int id = 0;
        int userId = 0;
        time_t date = 0;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("date", &Visit::date)));
    storage.sync_schema();
    
    int id = 1;
    User will{id++, "Will"};
    User smith{id++, "Smith"};
    User nicole{id++, "Nicole"};
    
    storage.replace(will);
    storage.replace(smith);
    storage.replace(nicole);
    
    id = 1;
    storage.replace(Visit{id++, will.id, 10});
    storage.replace(Visit{id++, will.id, 20});
    storage.replace(Visit{id++, will.id, 30});
    
    storage.replace(Visit{id++, smith.id, 25});
    storage.replace(Visit{id++, smith.id, 35});
    
    {
        auto rows = storage.get_all<User>(left_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(left_outer_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
    {
        auto rows = storage.get_all<User>(inner_join<Visit>(on(is_equal(&Visit::userId, 2))));
        REQUIRE(rows.size() == 6);
    }
}

TEST_CASE("Storage copy", "on_open"){
    int calledCount = 0;
    
    auto storage = make_storage({});
    
    storage.on_open = [&calledCount](sqlite3 *){
        ++calledCount;
    };
    
    storage.on_open(nullptr);
    REQUIRE(calledCount == 1);
    
    auto storageCopy = storage;
    REQUIRE(storageCopy.on_open);
    storageCopy.on_open(nullptr);
    REQUIRE(calledCount == 2);
}

TEST_CASE("Set null", "update_all") {
    
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    
    auto storage = make_storage({},
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.replace(User{1, std::make_unique<std::string>("Ototo")});
    REQUIRE(storage.count<User>() == 1);
    
    auto rows = storage.get_all<User>();
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front().name);
    
    storage.update_all(set(assign(&User::name, nullptr)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }
    
    storage.update_all(set(assign(&User::name, "ototo")));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front().name);
        REQUIRE(*rows.front().name == "ototo");
    }
    
    storage.update_all(set(assign(&User::name, nullptr)),
                       where(is_equal(&User::id, 1)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }
}

TEST_CASE("Composite key column names", "table"){
    
    struct User {
        int id = 0;
        std::string name;
        std::string info;
    };
    
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::id, &User::name));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"id", "name"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::name, &User::id));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"name", "id"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info),
                                primary_key(&User::name, &User::id, &User::info));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        std::vector<std::string> expected = {"name", "id", "info"};
        REQUIRE(std::equal(compositeKeyColumnsNames.begin(), compositeKeyColumnsNames.end(), expected.begin()));
    }
    {
        auto table = make_table("t",
                                make_column("id", &User::id),
                                make_column("name", &User::name),
                                make_column("info", &User::info));
        auto compositeKeyColumnsNames = table.composite_key_columns_names();
        REQUIRE(compositeKeyColumnsNames.empty());
    }
}

TEST_CASE("Not operator", "select"){
    struct Object {
        int id = 0;
    };
    
    auto storage = make_storage("",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key())));
    storage.sync_schema();
    
    storage.replace(Object{2});
    
    auto rows = storage.select(&Object::id, where(not is_equal(&Object::id, 1)));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 2);
}

TEST_CASE("Between operator", "select"){
    struct Object {
        int id = 0;
    };
    
    auto storage = make_storage("",
                                make_table("objects",
                                           make_column("id", &Object::id, autoincrement(), primary_key())));
    storage.sync_schema();
    
    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});
    storage.insert(Object{});
    
    auto allObjects = storage.get_all<Object>();
    auto rows = storage.select(&Object::id, where(between(&Object::id, 1, 3)));
    REQUIRE(rows.size() == 3);
}

TEST_CASE("Like operator", "select"){
    struct User {
        int id = 0;
        std::string name;
    };
    
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, autoincrement(), primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.insert(User{0, "Sia"});
    storage.insert(User{0, "Stark"});
    storage.insert(User{0, "Index"});
    
    auto whereCondition = where(like(&User::name, "S%"));
    {
        auto users = storage.get_all<User>(whereCondition);
        REQUIRE(users.size() == 2);
    }
    {
        auto rows = storage.select(&User::id, whereCondition);
        REQUIRE(rows.size() == 2);
    }
}

TEST_CASE("Exists", "select"){
    struct User {
        int id = 0;
        std::string name;
    };
    
    struct Visit {
        int id = 0;
        int userId = 0;
        time_t time = 0;
    };
    
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key()),
                                           make_column("userId", &Visit::userId),
                                           make_column("time", &Visit::time),
                                           foreign_key(&Visit::userId).references(&User::id)));
    storage.sync_schema();
    
    storage.replace(User{1, "Daddy Yankee"});
    storage.replace(User{2, "Don Omar"});
    
    storage.replace(Visit{1, 1, 100000});
    storage.replace(Visit{2, 1, 100001});
    storage.replace(Visit{3, 1, 100002});
    storage.replace(Visit{4, 1, 200000});
    
    storage.replace(Visit{5, 2, 100000});
    
    auto rows = storage.select(&User::id, where(exists(select(&Visit::id, where(c(&Visit::time) == 200000 and eq(&Visit::userId, &User::id))))));
    REQUIRE(!rows.empty() == 1);
}

TEST_CASE("Is null", "count"){
    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.replace(User{1, std::make_unique<std::string>("Sheldon")});
    storage.replace(User{2});
    storage.replace(User{3, std::make_unique<std::string>("Leonard")});
    
    REQUIRE(storage.count<User>() == 3);
    REQUIRE(storage.count<User>(where(is_null(&User::name))) == 1);
    REQUIRE(storage.count<User>(where(is_not_null(&User::name))) == 2);
}

TEST_CASE("Iterate blob", "iterate"){
    struct Test {
        int64_t id;
        std::vector<char> key;
    };
    
    auto db = make_storage("",
                           make_table("Test",
                                      make_column("key", &Test::key),
                                      make_column("id", &Test::id, primary_key())));
    db.sync_schema(true);
    
    std::vector<char> key(255);
    iota(key.begin(), key.end(), 0);
    
    Test v{5, key};
    
    db.replace(v);
    
    for(auto &obj : db.iterate<Test>()){
        cout << db.dump(obj) << endl;
    } //  test that view_t and iterator_t compile
    
    for(const auto &obj : db.iterate<Test>()){
        cout << db.dump(obj) << endl;
    } //  test that view_t and iterator_t compile
    
    {
        auto keysCount = db.count<Test>(where(c(&Test::key) == key));
        auto keysCountRows = db.select(count<Test>(), where(c(&Test::key) == key));
        REQUIRE(keysCountRows.size() == 1);
        REQUIRE(keysCountRows.front() == 1);
        REQUIRE(keysCount == keysCountRows.front());
        REQUIRE(db.get_all<Test>(where(c(&Test::key) == key)).size() == 1);
    }
    {
        int iterationsCount = 0;
        for (auto& w : db.iterate<Test>(where(c(&Test::key) == key))) {
            cout << w.id << endl;
            ++iterationsCount;
        }
        REQUIRE(iterationsCount == 1);
    }
}

TEST_CASE("Cast", "select"){
    struct Student {
        int id;
        float scoreFloat;
        std::string scoreString;
    };
    
    auto storage = make_storage("",
                                make_table("students",
                                           make_column("id", &Student::id, primary_key()),
                                           make_column("score_float", &Student::scoreFloat),
                                           make_column("score_str", &Student::scoreString)));
    storage.sync_schema();
    
    storage.replace(Student{1, 10.1f, "14.5"});
    
    {
        auto rows = storage.select(columns(cast<int>(&Student::scoreFloat), cast<int>(&Student::scoreString)));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(std::get<0>(row) == 10);
        REQUIRE(std::get<1>(row) == 14);
    }
    {
        auto rows = storage.select(cast<std::string>(5));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row == "5");
    }
}

TEST_CASE("Simple query", "select"){
    auto storage = make_storage("");
    {
        //  SELECT 1
        auto one = storage.select(1);
        REQUIRE(one.size() == 1);
        REQUIRE(one.front() == 1);
    }
    {
        //  SELECT 'ototo'
        auto ototo = storage.select("ototo");
        REQUIRE(ototo.size() == 1);
        REQUIRE(ototo.front() == "ototo");
    }
    {
        //  SELECT 1 + 1
        auto two = storage.select(c(1) + 1);
        REQUIRE(two.size() == 1);
        REQUIRE(two.front() == 2);
        
        auto twoAgain = storage.select(add(1, 1));
        REQUIRE(two == twoAgain);
    }
    {
        //  SELECT 10 / 5, 2 * 4
        auto math = storage.select(columns(sqlite_orm::div(10, 5), mul(2, 4)));
        REQUIRE(math.size() == 1);
        REQUIRE(math.front() == std::make_tuple(2, 8));
    }
    {
        //  SELECT 1, 2
        auto twoRows = storage.select(columns(1, 2));
        REQUIRE(twoRows.size() == 1);
        REQUIRE(std::get<0>(twoRows.front()) == 1);
        REQUIRE(std::get<1>(twoRows.front()) == 2);
    }
    {
        //  SELECT 1, 2
        //  UNION ALL
        //  SELECT 3, 4;
        auto twoRowsUnion = storage.select(union_all(select(columns(1, 2)), select(columns(3, 4))));
        REQUIRE(twoRowsUnion.size() == 2);
        REQUIRE(twoRowsUnion[0] == std::make_tuple(1, 2));
        REQUIRE(twoRowsUnion[1] == std::make_tuple(3, 4));
    }
}

TEST_CASE("Threadsafe", "threadsafe"){
    //  this code just shows this value on CI
    cout << "threadsafe = " << threadsafe() << endl;
}

TEST_CASE("In", "get_all"){
    {
        struct User {
            int id;
        };
        
        auto storage = make_storage("",
                                    make_table("users",
                                               make_column("id", &User::id, primary_key())));
        storage.sync_schema();
        storage.replace(User{ 1 });
        storage.replace(User{ 2 });
        storage.replace(User{ 3 });
        
        {
            auto rows = storage.get_all<User>(where(in(&User::id, {1, 2, 3})));
            REQUIRE(rows.size() == 3);
        }
        {
            std::vector<int> inArgument;
            inArgument.push_back(1);
            inArgument.push_back(2);
            inArgument.push_back(3);
            auto rows = storage.get_all<User>(where(in(&User::id, inArgument)));
            REQUIRE(rows.size() == 3);
        }
    }
    {
        struct Letter {
            int id;
            std::string name;
        };
        auto storage = make_storage("",
                                    make_table("letters",
                                               make_column("id", &Letter::id, primary_key()),
                                               make_column("name", &Letter::name)));
        storage.sync_schema();
        
        storage.replace(Letter{1, "A"});
        storage.replace(Letter{2, "B"});
        storage.replace(Letter{3, "C"});
        
        {
            auto letters = storage.get_all<Letter>(where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(letters.size() == 3);
        }
        {
            auto rows = storage.select(columns(&Letter::name), where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(rows.size() == 3);
        }
        {
            auto rows2 = storage.select(&Letter::name, where(in(&Letter::id, {1, 2, 3})));
            REQUIRE(rows2.size() == 3);
        }
    }
}

TEST_CASE("Different getters and setters"){
    struct User {
        int id;
        std::string name;
        
        int getIdByValConst() const {
            return this->id;
        }
        
        void setIdByVal(int id) {
            this->id = id;
        }
        
        std::string getNameByVal() {
            return this->name;
        }
        
        void setNameByConstRef(const std::string &name) {
            this->name = name;
        }
        
        const int& getConstIdByRefConst() const {
            return this->id;
        }
        
        void setIdByRef(int &id) {
            this->id = id;
        }
        
        const std::string& getConstNameByRefConst() const {
            return this->name;
        }
        
        void setNameByRef(std::string &name) {
            this->name = std::move(name);
        }
    };
    
    auto filename = "different.sqlite";
    auto storage0 = make_storage(filename,
                                 make_table("users",
                                            make_column("id", &User::id, primary_key()),
                                            make_column("name", &User::name)));
    auto storage1 = make_storage(filename,
                                 make_table("users",
                                            make_column("id", &User::getIdByValConst, &User::setIdByVal, primary_key()),
                                            make_column("name", &User::setNameByConstRef, &User::getNameByVal)));
    auto storage2 = make_storage(filename,
                                 make_table("users",
                                            make_column("id", &User::getConstIdByRefConst, &User::setIdByRef, primary_key()),
                                            make_column("name", &User::getConstNameByRefConst, &User::setNameByRef)));
    storage0.sync_schema();
    storage0.remove_all<User>();
    
    REQUIRE(storage0.count<User>() == 0);
    REQUIRE(storage1.count<User>() == 0);
    REQUIRE(storage2.count<User>() == 0);
    
    storage0.replace(User{ 1, "Da buzz" });
    
    REQUIRE(storage0.count<User>() == 1);
    REQUIRE(storage1.count<User>() == 1);
    REQUIRE(storage2.count<User>() == 1);
    
    {
        auto ids = storage0.select(&User::id);
        REQUIRE(ids.size() == 1);
        REQUIRE(ids.front() == 1);
        auto ids2 = storage1.select(&User::getIdByValConst);
        REQUIRE(ids == ids2);
        auto ids3 = storage1.select(&User::setIdByVal);
        REQUIRE(ids3 == ids2);
        auto ids4 = storage2.select(&User::getConstIdByRefConst);
        REQUIRE(ids4 == ids3);
        auto ids5 = storage2.select(&User::setIdByRef);
        REQUIRE(ids5 == ids4);
    }
    {
        auto ids = storage0.select(&User::id, where(is_equal(&User::name, "Da buzz")));
        REQUIRE(ids.size() == 1);
        REQUIRE(ids.front() == 1);
        auto ids2 = storage1.select(&User::getIdByValConst, where(is_equal(&User::setNameByConstRef, "Da buzz")));
        REQUIRE(ids == ids2);
        auto ids3 = storage1.select(&User::setIdByVal, where(is_equal(&User::getNameByVal, "Da buzz")));
        REQUIRE(ids3 == ids2);
        auto ids4 = storage2.select(&User::getConstIdByRefConst, where(is_equal(&User::getConstNameByRefConst, "Da buzz")));
        REQUIRE(ids4 == ids3);
        auto ids5 = storage2.select(&User::setIdByRef, where(is_equal(&User::setNameByRef, "Da buzz")));
        REQUIRE(ids5 == ids4);
    }
    {
        auto ids = storage0.select(columns(&User::id), where(is_equal(&User::name, "Da buzz")));
        REQUIRE(ids.size() == 1);
        REQUIRE(std::get<0>(ids.front()) == 1);
        auto ids2 = storage1.select(columns(&User::getIdByValConst), where(is_equal(&User::setNameByConstRef, "Da buzz")));
        REQUIRE(ids == ids2);
        auto ids3 = storage1.select(columns(&User::setIdByVal), where(is_equal(&User::getNameByVal, "Da buzz")));
        REQUIRE(ids3 == ids2);
        auto ids4 = storage2.select(columns(&User::getConstIdByRefConst), where(is_equal(&User::getConstNameByRefConst, "Da buzz")));
        REQUIRE(ids4 == ids3);
        auto ids5 = storage2.select(columns(&User::setIdByRef), where(is_equal(&User::setNameByRef, "Da buzz")));
        REQUIRE(ids5 == ids4);
    }
    {
        auto avgValue = storage0.avg(&User::id);
        REQUIRE(avgValue == storage1.avg(&User::getIdByValConst));
        REQUIRE(avgValue == storage1.avg(&User::setIdByVal));
        REQUIRE(avgValue == storage2.avg(&User::getConstIdByRefConst));
        REQUIRE(avgValue == storage2.avg(&User::setIdByRef));
    }
    {
        auto count = storage0.count(&User::id);
        REQUIRE(count == storage1.count(&User::getIdByValConst));
        REQUIRE(count == storage1.count(&User::setIdByVal));
        REQUIRE(count == storage2.count(&User::getConstIdByRefConst));
        REQUIRE(count == storage2.count(&User::setIdByRef));
    }
    {
        auto groupConcat = storage0.group_concat(&User::id);
        REQUIRE(groupConcat == storage1.group_concat(&User::getIdByValConst));
        REQUIRE(groupConcat == storage1.group_concat(&User::setIdByVal));
        REQUIRE(groupConcat == storage2.group_concat(&User::getConstIdByRefConst));
        REQUIRE(groupConcat == storage2.group_concat(&User::setIdByRef));
    }
    {
        auto arg = "ototo";
        auto groupConcat = storage0.group_concat(&User::id, arg);
        REQUIRE(groupConcat == storage1.group_concat(&User::getIdByValConst, arg));
        REQUIRE(groupConcat == storage1.group_concat(&User::setIdByVal, arg));
        REQUIRE(groupConcat == storage2.group_concat(&User::getConstIdByRefConst, arg));
        REQUIRE(groupConcat == storage2.group_concat(&User::setIdByRef, arg));
    }
    {
        auto max = storage0.max(&User::id);
        REQUIRE(max);
        REQUIRE(*max == *storage1.max(&User::getIdByValConst));
        REQUIRE(*max == *storage1.max(&User::setIdByVal));
        REQUIRE(*max == *storage2.max(&User::getConstIdByRefConst));
        REQUIRE(*max == *storage2.max(&User::setIdByRef));
    }
    {
        auto min = storage0.min(&User::id);
        REQUIRE(min);
        REQUIRE(*min == *storage1.min(&User::getIdByValConst));
        REQUIRE(*min == *storage1.min(&User::setIdByVal));
        REQUIRE(*min == *storage2.min(&User::getConstIdByRefConst));
        REQUIRE(*min == *storage2.min(&User::setIdByRef));
    }
    {
        auto sum = storage0.sum(&User::id);
        REQUIRE(sum);
        REQUIRE(*sum == *storage1.sum(&User::getIdByValConst));
        REQUIRE(*sum == *storage1.sum(&User::setIdByVal));
        REQUIRE(*sum == *storage2.sum(&User::getConstIdByRefConst));
        REQUIRE(*sum == *storage2.sum(&User::setIdByRef));
    }
    {
        auto total = storage0.total(&User::id);
        REQUIRE(total == storage1.total(&User::getIdByValConst));
        REQUIRE(total == storage1.total(&User::setIdByVal));
        REQUIRE(total == storage2.total(&User::getConstIdByRefConst));
        REQUIRE(total == storage2.total(&User::setIdByRef));
    }
}

TEST_CASE("Explicit colums"){
    struct Object {
        int id;
    };
    
    struct User : Object {
        std::string name;
        
        User(decltype(id) id_, decltype(name) name_): Object{id_}, name(std::move(name_)) {}
    };
    
    struct Token : Object {
        std::string token;
        int usedId;
        
        Token(decltype(id) id_, decltype(token) token_, decltype(usedId) usedId_): Object{id_}, token(std::move(token_)), usedId(usedId_) {}
    };
    
    auto storage = make_storage("column_pointer.sqlite",
                                make_table<User>("users",
                                                 make_column("id", &User::id, primary_key()),
                                                 make_column("name", &User::name)),
                                make_table<Token>("tokens",
                                                  make_column("id", &Token::id, primary_key()),
                                                  make_column("token", &Token::token),
                                                  make_column("used_id", &Token::usedId),
                                                  foreign_key(&Token::usedId).references(column<User>(&User::id))));
    storage.sync_schema();
    REQUIRE(storage.table_exists("users"));
    REQUIRE(storage.table_exists("tokens"));
    
    storage.remove_all<Token>();
    storage.remove_all<User>();
    
    auto brunoId = storage.insert(User{0, "Bruno"});
    auto zeddId = storage.insert(User{0, "Zedd"});
    
    REQUIRE(storage.count<User>() == 2);
    
    {
        auto w = where(is_equal(&User::name, "Bruno"));
        
        {
            auto rows = storage.select(column<User>(&User::id), w);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.front() == brunoId);
        }
        
        {
            auto rows2 = storage.select(columns(column<User>(&User::id)), w);
            REQUIRE(rows2.size() == 1);
            REQUIRE(std::get<0>(rows2.front()) == brunoId);
            
            auto rows3 = storage.select(columns(column<User>(&Object::id)), w);
            REQUIRE(rows3 == rows2);
        }
    }
    
    {
        auto rows = storage.select(column<User>(&User::id), where(is_equal(&User::name, "Zedd")));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == zeddId);
    }
    
    {
        auto abcId = storage.insert(Token(0, "abc", brunoId));
        
        auto w = where(is_equal(&Token::token, "abc"));
        {
            auto rows = storage.select(column<Token>(&Token::id), w);
            REQUIRE(rows.size() == 1);
            REQUIRE(rows.front() == abcId);
        }
        
        {
            auto rows2 = storage.select(columns(column<Token>(&Token::id), &Token::usedId), w);
            REQUIRE(rows2.size() == 1);
            REQUIRE(std::get<0>(rows2.front()) == abcId);
            REQUIRE(std::get<1>(rows2.front()) == brunoId);
        }
    }
    
    {
        auto joinedRows = storage.select(columns(&User::name, &Token::token),
                                         join<Token>(on(is_equal(&Token::usedId, column<User>(&User::id)))));
        REQUIRE(joinedRows.size() == 1);
        REQUIRE(std::get<0>(joinedRows.front()) == "Bruno");
        REQUIRE(std::get<1>(joinedRows.front()) == "abc");
    }
}

TEST_CASE("Join iterator ctor compilation error") {
    //  TODO: move to static tests
    struct Tag {
        int objectId;
        std::string text;
    };
    
    auto storage = make_storage("join_error.sqlite",
                                make_table("tags",
                                           make_column("object_id", &Tag::objectId),
                                           make_column("text", &Tag::text)));
    storage.sync_schema();
    
    auto offs = 0;
    auto lim = 5;
    storage.select(columns(&Tag::text, count(&Tag::text)),
                   group_by(&Tag::text),
                   order_by(count(&Tag::text)).desc(),
                   limit(offs, lim));
}

TEST_CASE("limits"){
    auto storage2 = make_storage("limits.sqlite");
    auto storage = storage2;
    storage.sync_schema();

    {
        auto length = storage.limit.length();
        auto newLength = length - 10;
        storage.limit.length(newLength);
        length = storage.limit.length();
        REQUIRE(length == newLength);
    }
    {
        auto sqlLength = storage.limit.sql_length();
        auto newSqlLength = sqlLength - 10;
        storage.limit.sql_length(newSqlLength);
        sqlLength = storage.limit.sql_length();
        REQUIRE(sqlLength == newSqlLength);
    }
    {
        auto column = storage.limit.column();
        auto newColumn = column - 10;
        storage.limit.column(newColumn);
        column = storage.limit.column();
        REQUIRE(column == newColumn);
    }
    {
        auto exprDepth = storage.limit.expr_depth();
        auto newExprDepth = exprDepth - 10;
        storage.limit.expr_depth(newExprDepth);
        exprDepth = storage.limit.expr_depth();
        REQUIRE(exprDepth == newExprDepth);
    }
    {
        auto compoundSelect = storage.limit.compound_select();
        auto newCompoundSelect = compoundSelect - 10;
        storage.limit.compound_select(newCompoundSelect);
        compoundSelect = storage.limit.compound_select();
        REQUIRE(compoundSelect == newCompoundSelect);
    }
    {
        auto vdbeOp = storage.limit.vdbe_op();
        auto newVdbe_op = vdbeOp - 10;
        storage.limit.vdbe_op(newVdbe_op);
        vdbeOp = storage.limit.vdbe_op();
        REQUIRE(vdbeOp == newVdbe_op);
    }
    {
        auto functionArg = storage.limit.function_arg();
        auto newFunctionArg = functionArg - 10;
        storage.limit.function_arg(newFunctionArg);
        functionArg = storage.limit.function_arg();
        REQUIRE(functionArg == newFunctionArg);
    }
    {
        auto attached = storage.limit.attached();
        auto newAttached = attached - 1;
        storage.limit.attached(newAttached);
        attached = storage.limit.attached();
        REQUIRE(attached == newAttached);
    }
    {
        auto likePatternLength = storage.limit.like_pattern_length();
        auto newLikePatternLength = likePatternLength - 10;
        storage.limit.like_pattern_length(newLikePatternLength);
        likePatternLength = storage.limit.like_pattern_length();
        REQUIRE(likePatternLength == newLikePatternLength);
    }
    {
        auto variableNumber = storage.limit.variable_number();
        auto newVariableNumber = variableNumber - 10;
        storage.limit.variable_number(newVariableNumber);
        variableNumber = storage.limit.variable_number();
        REQUIRE(variableNumber == newVariableNumber);
    }
    {
        auto triggerDepth = storage.limit.trigger_depth();
        auto newTriggerDepth = triggerDepth - 10;
        storage.limit.trigger_depth(newTriggerDepth);
        triggerDepth = storage.limit.trigger_depth();
        REQUIRE(triggerDepth == newTriggerDepth);
    }
    {
        auto workerThreads = storage.limit.worker_threads();
        auto newWorkerThreads = workerThreads + 1;
        storage.limit.worker_threads(newWorkerThreads);
        workerThreads = storage.limit.worker_threads();
        REQUIRE(workerThreads == newWorkerThreads);
    }
}

TEST_CASE("Explicit insert"){
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
    
    auto storage = make_storage("explicitinsert.sqlite",
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
    
    {
        
        {
            User user{};
            user.name = "Juan";
            user.age = 57;
            auto id = storage.insert(user, columns(&User::name, &User::age));
            REQUIRE(storage.get<User>(id).email == "dummy@email.com");
        }
        
        {
            User user2;
            user2.id = 2;
            user2.name = "Kevin";
            user2.age = 27;
            REQUIRE(user2.id == storage.insert(user2, columns(&User::id, &User::name, &User::age)));
            REQUIRE(storage.get<User>(user2.id).email == "dummy@email.com");
        }
        
        {
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
        
        {
            User user4;
            user4.name = "Egor";
            try {
                storage.insert(user4, columns(&User::name));
                REQUIRE(false);
//                throw std::runtime_error("Must not fire");
            } catch (const std::system_error&) {
                //        cout << e.what() << endl;
            }
        }
    }
    {
        
        {
            Visit visit;
            
            {
                visit.setUsedId(1);
                visit.setId(storage.insert(visit, columns(&Visit::usedId)));
                
                auto visitFromStorage = storage.get<Visit>(visit.id());
                REQUIRE(visitFromStorage.createdAt() == 10);
                REQUIRE(visitFromStorage.usedId() == visit.usedId());
                storage.remove<Visit>(visitFromStorage.usedId());
            }
            
            {
                visit.setId(storage.insert(visit, columns(&Visit::setUsedId)));
                auto visitFromStorage = storage.get<Visit>(visit.id());
                REQUIRE(visitFromStorage.createdAt() == 10);
                REQUIRE(visitFromStorage.usedId() == visit.usedId());
                storage.remove<Visit>(visitFromStorage.usedId());
            }
            
            {
                Visit visit2;
                visit2.setId(2);
                visit2.setUsedId(1);
                {
                    auto insertedId = storage.insert(visit2, columns(&Visit::id, &Visit::usedId));
                    REQUIRE(visit2.id() == insertedId);
                    auto visitFromStorage = storage.get<Visit>(visit2.id());
                    REQUIRE(visitFromStorage.usedId() == visit2.usedId());
                    storage.remove<Visit>(visit2.id());
                }
                {
                    auto insertedId = storage.insert(visit2, columns(&Visit::setId, &Visit::setUsedId));
                    REQUIRE(visit2.id() == insertedId);
                    auto visitFromStorage = storage.get<Visit>(visit2.id());
                    REQUIRE(visitFromStorage.usedId() == visit2.usedId());
                    storage.remove<Visit>(visit2.id());
                }
            }
            
            {
                Visit visit3;
                visit3.setId(10);
                try {
                    storage.insert(visit3, columns(&Visit::id));
                    REQUIRE(false);
                } catch (const std::system_error&) {
                    //        cout << e.what() << endl;
                }
                
                try {
                    storage.insert(visit3, columns(&Visit::setId));
                    REQUIRE(false);
                } catch (const std::system_error&) {
                    //        cout << e.what() << endl;
                }
            }
        }
    }
}

TEST_CASE("Custom collate"){
    struct Item {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("custom_collate.sqlite",
                                make_table("items",
                                           make_column("id", &Item::id, primary_key()),
                                           make_column("name", &Item::name)));
//    storage.open_forever();
    storage.sync_schema();
    storage.remove_all<Item>();
    storage.insert(Item{ 0, "Mercury" });
    storage.insert(Item{ 0, "Mars" });
    storage.create_collation("ototo", [](int, const void *lhs, int, const void *rhs){
        return strcmp((const char*)lhs, (const char*)rhs);
    });
    storage.create_collation("alwaysequal", [](int, const void *, int, const void *){
        return 0;
    });
    auto rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "Mercury");
    
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("ototo"));
    
    storage.create_collation("ototo", {});
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo")));
    } catch (const std::system_error& e) {
        cout << e.what() << endl;
    }
    try {
        rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("ototo2")));
    } catch (const std::system_error& e) {
        cout << e.what() << endl;
    }
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                                             order_by(&Item::name).collate_rtrim());
    
    rows = storage.select(&Item::name, where(is_equal(&Item::name, "Mercury").collate("alwaysequal")),
                          order_by(&Item::name).collate("alwaysequal"));
    REQUIRE(rows.size() == static_cast<size_t>(storage.count<Item>()));
}

TEST_CASE("Vacuum"){
    struct Item {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("vacuum.sqlite",
                                make_table("items",
                                           make_column("id", &Item::id, primary_key()),
                                           make_column("name", &Item::name)));
    storage.sync_schema();
    storage.insert(Item{ 0, "One" });
    storage.insert(Item{ 0, "Two" });
    storage.insert(Item{ 0, "Three" });
    storage.insert(Item{ 0, "Four" });
    storage.insert(Item{ 0, "Five" });
    storage.remove_all<Item>();
    storage.vacuum();
}

TEST_CASE("Remove all"){
    struct Object {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key()),
                                           make_column("name", &Object::name)));
    storage.sync_schema();
    
    storage.replace(Object{ 1, "Ototo" });
    storage.replace(Object{ 2, "Contigo" });
    
    REQUIRE(storage.count<Object>() == 2);
    
    storage.remove_all<Object>(where(c(&Object::id) == 1));
    
    REQUIRE(storage.count<Object>() == 1);
}

TEST_CASE("Escaped index name"){
    struct User{
        std::string group;
    };
    auto storage = make_storage("index_group.sqlite",
                                make_index("index", &User::group),
                                make_table("users",
                                           make_column("group", &User::group)));
    storage.sync_schema();
}

TEST_CASE("Where"){
    struct User{
        int id = 0;
        std::string name;
    };
    
    auto storage = make_storage("",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.replace(User{ 1, "Jeremy" });
    storage.replace(User{ 2, "Nataly" });
    
    auto users = storage.get_all<User>();
    REQUIRE(users.size() == 2);
    
    auto users2 = storage.get_all<User>(where(true));
    REQUIRE(users2.size() == 2);
    
    auto users3 = storage.get_all<User>(where(false));
    REQUIRE(users3.size() == 0);
    
    auto users4 = storage.get_all<User>(where(true and c(&User::id) == 1));
    REQUIRE(users4.size() == 1);
    REQUIRE(users4.front().id == 1);
    
    auto users5 = storage.get_all<User>(where(false and c(&User::id) == 1));
    REQUIRE(users5.size() == 0);
}
