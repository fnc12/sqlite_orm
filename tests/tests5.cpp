#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#include <numeric>  //  std::iota

using namespace sqlite_orm;

TEST_CASE("Iterate blob") {
    struct Test {
        int64_t id;
        std::vector<char> key;
    };

    struct TestComparator {
        bool operator()(const Test& lhs, const Test& rhs) const {
            return lhs.id == rhs.id && lhs.key == rhs.key;
        }
    };

    auto db =
        make_storage("",
                     make_table("Test", make_column("key", &Test::key), make_column("id", &Test::id, primary_key())));
    db.sync_schema(true);

    std::vector<char> key(255);
    iota(key.begin(), key.end(), '\0');

    Test v{5, key};

    db.replace(v);

    TestComparator testComparator;
    for(auto& obj: db.iterate<Test>()) {
        REQUIRE(testComparator(obj, v));
    }  //  test that view_t and iterator_t compile

    for(const auto& obj: db.iterate<Test>()) {
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
        for(auto& w: db.iterate<Test>(where(c(&Test::key) == key))) {
            REQUIRE(testComparator(w, v));
            ++iterationsCount;
        }
        REQUIRE(iterationsCount == 1);
    }
}

TEST_CASE("Threadsafe") {
    threadsafe();
}

TEST_CASE("Different getters and setters") {
    struct User {
        int id;
        std::string name;

        int getIdByValConst() const {
            return this->id;
        }

        void setIdByVal(int id_) {
            this->id = id_;
        }

        std::string getNameByVal() {
            return this->name;
        }

        void setNameByConstRef(const std::string& name_) {
            this->name = name_;
        }

        const int& getConstIdByRefConst() const {
            return this->id;
        }

        void setIdByRef(int& id_) {
            this->id = id_;
        }

        const std::string& getConstNameByRefConst() const {
            return this->name;
        }

        void setNameByRef(std::string& name_) {
            this->name = std::move(name_);
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

TEST_CASE("issue730") {
    struct Table {
        int64_t id;
        std::string a;
        std::string b;
        std::string c;
    };
    auto storage = make_storage({},
                                make_table("table",
                                           make_column("id", &Table::id),
                                           make_column("a", &Table::a),
                                           make_column("b", &Table::b),
                                           make_column("c", &Table::c),
                                           sqlite_orm::unique(&Table::a, &Table::b, &Table::c)));
    storage.sync_schema();

    auto rows = storage.select(asterisk<Table>());

    using Rows = decltype(rows);
    using ExpectedRows = std::vector<std::tuple<int64_t, std::string, std::string, std::string>>;

    STATIC_REQUIRE(std::is_same<Rows, ExpectedRows>::value);
}

TEST_CASE("issue822") {
    class A {
      public:
        A() = default;
        A(const uint8_t& address, const uint8_t& type, const uint8_t& idx, std::shared_ptr<double> value) :
            address(address), type(type), idx(idx), value(std::move(value)){};

        const uint8_t& getAddress() const {
            return this->address;
        }

        void setAddress(const uint8_t& address) {
            this->address = address;
        }

        const uint8_t& getType() const {
            return this->type;
        }

        void setType(const uint8_t& type) {
            this->type = type;
        }

        const uint8_t& getIndex() const {
            return this->idx;
        }

        void setIndex(const uint8_t& index) {
            this->idx = index;
        }

        std::shared_ptr<double> getValue() const {
            return this->value;
        }

        void setValue(std::shared_ptr<double> value) {
            this->value = std::move(value);
        }

      private:
        uint8_t address;
        uint8_t type;
        uint8_t idx;
        std::shared_ptr<double> value;
    };
    auto storage = make_storage("",
                                make_table("A",
                                           make_column("address", &A::getAddress, &A::setAddress),
                                           make_column("type", &A::getType, &A::setType),
                                           make_column("idx", &A::getIndex, &A::setIndex),
                                           make_column("value", &A::getValue, &A::setValue),
                                           primary_key(&A::getAddress, &A::getType, &A::getIndex)));
    storage.sync_schema();
    storage.replace(A(1, 1, 0, std::make_shared<double>(55.5)));
    auto records = storage.get_all<A>(where(c(&A::getAddress) == 1 and c(&A::getType) == 1 and c(&A::getIndex) == 0));
    if(records.size() != 0) {
        A a = records[0];
        a.setValue(std::make_shared<double>(10));
        storage.update(a);
        records = storage.get_all<A>(where(c(&A::getAddress) == 1 and c(&A::getType) == 1 and c(&A::getIndex) == 0));
        REQUIRE(records.size() == 1);
        REQUIRE(*records[0].getValue() == 10);
    }
}
