#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <numeric>
#include <algorithm>  //  std::count_if
#include <iostream>
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Case") {

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

        auto rows = storage.select(
            columns(case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end()),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<User>());
            REQUIRE(std::get<0>(rows[0]) == "Foreign");
            REQUIRE(std::get<0>(rows[1]) == "Foreign");
            REQUIRE(std::get<0>(rows[2]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[3]) == "Dosmetic");
            REQUIRE(std::get<0>(rows[4]) == "Dosmetic");
        };
        verifyRows(rows);

        rows = storage.select(
            columns(as<GradeAlias>(
                case_<std::string>(&User::country).when("USA", then("Dosmetic")).else_("Foreign").end())),
            multi_order_by(order_by(&User::lastName), order_by(&User::firstName)));

        verifyRows(rows);
    }
    {
        storage.insert(Track{0, "For Those About To Rock", 400000});
        storage.insert(Track{0, "Balls to the Wall", 500000});
        storage.insert(Track{0, "Fast as a Shark", 200000});
        storage.insert(Track{0, "Restless and Wild", 100000});
        storage.insert(Track{0, "Princess of the Dawn", 50000});

        auto rows = storage.select(
            case_<std::string>()
                .when(c(&Track::milliseconds) < 60000, then("short"))
                .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                .else_("long")
                .end(),
            order_by(&Track::name));
        auto verifyRows = [&storage](auto &rows) {
            REQUIRE(rows.size() == storage.count<Track>());
            REQUIRE(rows[0] == "long");
            REQUIRE(rows[1] == "medium");
            REQUIRE(rows[2] == "long");
            REQUIRE(rows[3] == "short");
            REQUIRE(rows[4] == "medium");
        };
        verifyRows(rows);

        rows = storage.select(
            as<GradeAlias>(
                case_<std::string>()
                    .when(c(&Track::milliseconds) < 60000, then("short"))
                    .when(c(&Track::milliseconds) >= 60000 and c(&Track::milliseconds) < 300000, then("medium"))
                    .else_("long")
                    .end()),
            order_by(&Track::name));
        verifyRows(rows);
    }
}

TEST_CASE("Unique ptr in update") {

    struct User {
        int id = 0;
        std::unique_ptr<std::string> name;
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
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

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("Optional in update") {

    struct User {
        int id = 0;
        std::optional<int> carYear;  // will be empty if user takes the bus.
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("car_year", &User::carYear)));
    storage.sync_schema();

    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{});
    storage.insert(User{0, 2006});

    REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 1);

    {
        storage.update_all(set(assign(&User::carYear, std::optional<int>{})));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, 1994)));
        REQUIRE(storage.count<User>(where(is_null(&User::carYear))) == 0);
    }
    {
        storage.update_all(set(assign(&User::carYear, nullptr)));
        REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 0);
    }
}
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED

TEST_CASE("Join") {

    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        int id = 0;
        int userId = 0;
        time_t date = 0;
    };

    auto storage =
        make_storage({},
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
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
    {
        auto rows = storage.get_all<User>(cross_join<Visit>());
        REQUIRE(rows.size() == 15);
    }
    {
        auto rows = storage.get_all<User>(natural_join<Visit>());
        REQUIRE(rows.size() == 3);
    }
}

TEST_CASE("Storage copy") {
    struct User {
        int id = 0;
    };

    int calledCount = 0;

    auto storage = make_storage({}, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();
    storage.remove_all<User>();

    storage.on_open = [&calledCount](sqlite3 *) {
        ++calledCount;
    };

    storage.on_open(nullptr);
    REQUIRE(calledCount == 1);

    auto storageCopy = storage;
    REQUIRE(storageCopy.on_open);
    REQUIRE(calledCount == 2);
    storageCopy.on_open(nullptr);
    REQUIRE(calledCount == 3);

    storageCopy.sync_schema();
    storageCopy.remove_all<User>();
}

TEST_CASE("Set null") {

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

    storage.update_all(set(assign(&User::name, nullptr)), where(is_equal(&User::id, 1)));
    {
        auto rows = storage.get_all<User>();
        REQUIRE(rows.size() == 1);
        REQUIRE(!rows.front().name);
    }
}

TEST_CASE("Composite key column names") {

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

TEST_CASE("Not operator") {
    struct Object {
        int id = 0;
    };

    auto storage = make_storage("", make_table("objects", make_column("id", &Object::id, primary_key())));
    storage.sync_schema();

    storage.replace(Object{2});

    auto rows = storage.select(&Object::id, where(not is_equal(&Object::id, 1)));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 2);
}

TEST_CASE("Exists") {
    struct User {
        int id = 0;
        std::string name;
    };

    struct Visit {
        int id = 0;
        int userId = 0;
        time_t time = 0;
    };

    auto storage =
        make_storage("",
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
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

    auto rows = storage.select(
        &User::id,
        where(exists(select(&Visit::id, where(c(&Visit::time) == 200000 and eq(&Visit::userId, &User::id))))));
    REQUIRE(!rows.empty() == 1);
}

TEST_CASE("Iterate blob") {
    struct Test {
        int64_t id;
        std::vector<char> key;
    };

    struct TestComparator {
        bool operator()(const Test &lhs, const Test &rhs) const {
            return lhs.id == rhs.id && lhs.key == rhs.key;
        }
    };

    auto db =
        make_storage("",
                     make_table("Test", make_column("key", &Test::key), make_column("id", &Test::id, primary_key())));
    db.sync_schema(true);

    std::vector<char> key(255);
    iota(key.begin(), key.end(), 0);

    Test v{5, key};

    db.replace(v);

    TestComparator testComparator;
    for(auto &obj: db.iterate<Test>()) {
        REQUIRE(testComparator(obj, v));
    }  //  test that view_t and iterator_t compile

    for(const auto &obj: db.iterate<Test>()) {
        REQUIRE(testComparator(obj, v));
    }  //  test that view_t and iterator_t compile

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
        for(auto &w: db.iterate<Test>(where(c(&Test::key) == key))) {
            REQUIRE(testComparator(w, v));
            ++iterationsCount;
        }
        REQUIRE(iterationsCount == 1);
    }
}

TEST_CASE("Threadsafe") {
    //  this code just shows this value on CI
    cout << "threadsafe = " << threadsafe() << endl;
}

TEST_CASE("Different getters and setters") {
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

        const int &getConstIdByRefConst() const {
            return this->id;
        }

        void setIdByRef(int &id) {
            this->id = id;
        }

        const std::string &getConstNameByRefConst() const {
            return this->name;
        }

        void setNameByRef(std::string &name) {
            this->name = std::move(name);
        }
    };

    auto filename = "different.sqlite";
    auto storage0 = make_storage(
        filename,
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    auto storage1 = make_storage(filename,
                                 make_table("users",
                                            make_column("id", &User::getIdByValConst, &User::setIdByVal, primary_key()),
                                            make_column("name", &User::setNameByConstRef, &User::getNameByVal)));
    auto storage2 =
        make_storage(filename,
                     make_table("users",
                                make_column("id", &User::getConstIdByRefConst, &User::setIdByRef, primary_key()),
                                make_column("name", &User::getConstNameByRefConst, &User::setNameByRef)));
    storage0.sync_schema();
    storage0.remove_all<User>();

    REQUIRE(storage0.count<User>() == 0);
    REQUIRE(storage1.count<User>() == 0);
    REQUIRE(storage2.count<User>() == 0);

    storage0.replace(User{1, "Da buzz"});

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
        auto ids4 =
            storage2.select(&User::getConstIdByRefConst, where(is_equal(&User::getConstNameByRefConst, "Da buzz")));
        REQUIRE(ids4 == ids3);
        auto ids5 = storage2.select(&User::setIdByRef, where(is_equal(&User::setNameByRef, "Da buzz")));
        REQUIRE(ids5 == ids4);
    }
    {
        auto ids = storage0.select(columns(&User::id), where(is_equal(&User::name, "Da buzz")));
        REQUIRE(ids.size() == 1);
        REQUIRE(std::get<0>(ids.front()) == 1);
        auto ids2 =
            storage1.select(columns(&User::getIdByValConst), where(is_equal(&User::setNameByConstRef, "Da buzz")));
        REQUIRE(ids == ids2);
        auto ids3 = storage1.select(columns(&User::setIdByVal), where(is_equal(&User::getNameByVal, "Da buzz")));
        REQUIRE(ids3 == ids2);
        auto ids4 = storage2.select(columns(&User::getConstIdByRefConst),
                                    where(is_equal(&User::getConstNameByRefConst, "Da buzz")));
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

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("Dump") {

    struct User {
        int id = 0;
        std::optional<int> carYear;  // will be empty if user takes the bus.
    };

    auto storage = make_storage(
        {},
        make_table("users", make_column("id", &User::id, primary_key()), make_column("car_year", &User::carYear)));
    storage.sync_schema();

    auto userId_1 = storage.insert(User{0, {}});
    auto userId_2 = storage.insert(User{0, 2006});
    std::ignore = userId_2;

    REQUIRE(storage.count<User>(where(is_not_null(&User::carYear))) == 1);

    auto rows = storage.select(&User::carYear, where(is_equal(&User::id, userId_1)));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().has_value());

    auto allUsers = storage.get_all<User>();
    REQUIRE(allUsers.size() == 2);

    const std::string dumpUser1 = storage.dump(allUsers[0]);
    REQUIRE(dumpUser1 == std::string{"{ id : '1', car_year : 'null' }"});

    const std::string dumpUser2 = storage.dump(allUsers[1]);
    REQUIRE(dumpUser2 == std::string{"{ id : '2', car_year : '2006' }"});
}
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
