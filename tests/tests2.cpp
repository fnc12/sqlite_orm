#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>
#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
#include <optional>  // std::optional
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#include <iostream>

using namespace sqlite_orm;

using std::cout;
using std::endl;

TEST_CASE("Empty storage") {
    auto storage = make_storage("empty.sqlite");
    storage.table_exists("table");
}

TEST_CASE("Remove") {
    struct Object {
        int id;
        std::string name;
    };

    {
        auto storage = make_storage(
            "test_remove.sqlite",
            make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)));

        storage.sync_schema();
        storage.remove_all<Object>();

        auto id1 = storage.insert(Object{0, "Skillet"});
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

        auto id1 = storage.insert(Object{0, "Skillet"});
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

TEST_CASE("Select") {
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
    if(rc != SQLITE_DONE) {
        //        cout << sqlite3_errmsg(db) << endl;
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
    if(rc != SQLITE_DONE) {
        //        cout << sqlite3_errmsg(db) << endl;
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
    if(rc != SQLITE_DONE) {
        //        cout << sqlite3_errmsg(db) << endl;
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
        if(rc != SQLITE_ROW) {
            //            cout << sqlite3_errmsg(db) << endl;
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

    auto cols = columns(&Word::id, &Word::currentWord, &Word::beforeWord, &Word::afterWord, &Word::occurances);
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

    storage.update_all(set(assign(&Word::currentWord, "ototo")), where(is_equal(&Word::id, firstId)));

    REQUIRE(storage.get<Word>(firstId).currentWord == "ototo");
}

TEST_CASE("Replace query") {
    struct Object {
        int id;
        std::string name;
    };

    struct User {

        User(int id_, std::string name_) : id(id_), name(move(name_)) {}

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
            this->name = move(name_);
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
        vector.push_back(std::make_unique<Object>(Object{300, "Iggy"}));
        vector.push_back(std::make_unique<Object>(Object{400, "Azalea"}));
        storage.replace_range<Object>(vector.begin(),
                                      vector.end(),
                                      [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                          return *pointer;
                                      });
        REQUIRE(storage.count<Object>() == 4);

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        storage.replace_range<Object>(emptyVector.begin(),
                                      emptyVector.end(),
                                      [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                          return *pointer;
                                      });
    }
    REQUIRE(storage.count<User>() == 0);
    storage.replace(User{10, "Daddy Yankee"});
}

TEST_CASE("Insert") {
    struct Object {
        int id;
        std::string name;
    };

    struct ObjectWithoutRowid {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "test_insert.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
        make_table("objects_without_rowid",
                   make_column("id", &ObjectWithoutRowid::id, primary_key()),
                   make_column("name", &ObjectWithoutRowid::name))
            .without_rowid());

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

    auto countBefore = storage.count<Object>();
    SECTION("straight") {
        storage.insert_range(initList.begin(), initList.end());
        REQUIRE(storage.count<Object>() == countBefore + static_cast<int>(initList.size()));

        //  test empty container
        std::vector<Object> emptyVector;
        storage.insert_range(emptyVector.begin(), emptyVector.end());
    }
    SECTION("pointers") {
        std::vector<std::unique_ptr<Object>> pointers;
        pointers.reserve(initList.size());
        std::transform(initList.begin(), initList.end(), std::back_inserter(pointers), [](const Object &object) {
            return std::make_unique<Object>(Object{object});
        });
        storage.insert_range<Object>(pointers.begin(),
                                     pointers.end(),
                                     [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                         return *pointer;
                                     });

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        storage.insert_range<Object>(emptyVector.begin(),
                                     emptyVector.end(),
                                     [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                         return *pointer;
                                     });
    }

    //  test insert without rowid
    storage.insert(ObjectWithoutRowid{10, "Life"});
    REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
    storage.insert(ObjectWithoutRowid{20, "Death"});
    REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
}

struct SqrtFunction {
    static int callsCount;

    double operator()(double arg) const {
        ++callsCount;
        return std::sqrt(arg);
    }

    static const char *name() {
        return "SQRT";
    }
};

int SqrtFunction::callsCount = 0;

struct HasPrefixFunction {
    static int callsCount;
    static int objectsCount;

    HasPrefixFunction() {
        ++objectsCount;
    }

    HasPrefixFunction(const HasPrefixFunction &) {
        ++objectsCount;
    }

    HasPrefixFunction(HasPrefixFunction &&) {
        ++objectsCount;
    }

    ~HasPrefixFunction() {
        --objectsCount;
    }

    bool operator()(const std::string &str, const std::string &prefix) {
        ++callsCount;
        return str.compare(0, prefix.size(), prefix) == 0;
    }

    static std::string name() {
        return "HAS_PREFIX";
    }
};

int HasPrefixFunction::callsCount = 0;
int HasPrefixFunction::objectsCount = 0;

struct MeanFunction {
    double total = 0;
    int count = 0;

    static int objectsCount;

    MeanFunction() {
        ++objectsCount;
    }

    MeanFunction(const MeanFunction &) {
        ++objectsCount;
    }

    MeanFunction(MeanFunction &&) {
        ++objectsCount;
    }

    ~MeanFunction() {
        --objectsCount;
    }

    void step(double value) {
        total += value;
        ++count;
    }

    double fin() const {
        return total / count;
    }

    static std::string name() {
        return "MEAN";
    }
};

int MeanFunction::objectsCount = 0;

void printString(const std::string &row) {
    cout << row << " ( ";
    for(auto c: row) {
        cout << int(c) << ' ';
    }
    cout << ")" << endl;
}

void printArray(const std::vector<std::string> &array, const char *title) {
    cout << title << endl;
    for(auto &row: array) {
        printString(row);
    }
};

struct FirstFunction {
    static int objectsCount;
    static int callsCount;

    FirstFunction() {
        ++objectsCount;
    }

    FirstFunction(const MeanFunction &) {
        ++objectsCount;
    }

    FirstFunction(MeanFunction &&) {
        ++objectsCount;
    }

    ~FirstFunction() {
        --objectsCount;
    }

    std::string operator()(const arg_values &args) const {
        ++callsCount;
        std::string res;
        res.reserve(args.size());
        for(auto value: args) {
            auto stringValue = value.get<std::string>();
            cout << "FirstFunction::operator() *" << stringValue << "*" << endl;
            printString(stringValue);
            if(!stringValue.empty()) {
                res += stringValue.front();
            }
        }
        cout << "FirstFunction::operator() result =*" << res << "*" << endl;
        return res;
    }

    static const char *name() {
        return "FIRST";
    }
};

struct MultiSum {
    double sum = 0;

    static int objectsCount;

    MultiSum() {
        ++objectsCount;
    }

    MultiSum(const MeanFunction &) {
        ++objectsCount;
    }

    MultiSum(MeanFunction &&) {
        ++objectsCount;
    }

    ~MultiSum() {
        --objectsCount;
    }

    void step(const arg_values &args) {
        for(auto it = args.begin(); it != args.end(); ++it) {
            if(!it->empty() && (it->is_integer() || it->is_float())) {
                this->sum += it->get<double>();
            }
        }
    }

    double fin() const {
        return this->sum;
    }

    static const char *name() {
        return "MULTI_SUM";
    }
};

int MultiSum::objectsCount = 0;

int FirstFunction::objectsCount = 0;
int FirstFunction::callsCount = 0;

TEST_CASE("custom functions") {
    SqrtFunction::callsCount = 0;
    HasPrefixFunction::callsCount = 0;
    FirstFunction::callsCount = 0;

    std::string path;
    SECTION("in memory") {
        path = {};
    }
    SECTION("file") {
        path = "custom_function.sqlite";
        ::remove(path.c_str());
    }
    struct User {
        int id = 0;
    };
    auto storage = make_storage(path, make_table("users", make_column("id", &User::id)));
    storage.sync_schema();
    {  //   call before creation
        try {
            auto rows = storage.select(func<SqrtFunction>(4));
            REQUIRE(false);
        } catch(const std::system_error &) {
            //..
        }
    }

    //  create function
    REQUIRE(SqrtFunction::callsCount == 0);

    storage.create_scalar_function<SqrtFunction>();

    REQUIRE(SqrtFunction::callsCount == 0);

    //  call after creation
    {
        auto rows = storage.select(func<SqrtFunction>(4));
        REQUIRE(SqrtFunction::callsCount == 1);
        decltype(rows) expected;
        expected.push_back(2);
        REQUIRE(rows == expected);
    }

    //  create function
    REQUIRE(HasPrefixFunction::callsCount == 0);
    REQUIRE(HasPrefixFunction::objectsCount == 0);
    storage.create_scalar_function<HasPrefixFunction>();
    REQUIRE(HasPrefixFunction::callsCount == 0);
    REQUIRE(HasPrefixFunction::objectsCount == 0);

    //  call after creation
    {
        auto rows = storage.select(func<HasPrefixFunction>("one", "o"));
        decltype(rows) expected;
        expected.push_back(true);
        REQUIRE(rows == expected);
    }
    REQUIRE(HasPrefixFunction::callsCount == 1);
    REQUIRE(HasPrefixFunction::objectsCount == 0);
    {
        auto rows = storage.select(func<HasPrefixFunction>("two", "b"));
        decltype(rows) expected;
        expected.push_back(false);
        REQUIRE(rows == expected);
    }
    REQUIRE(HasPrefixFunction::callsCount == 2);
    REQUIRE(HasPrefixFunction::objectsCount == 0);

    //  delete function
    storage.delete_scalar_function<HasPrefixFunction>();

    //  delete function
    storage.delete_scalar_function<SqrtFunction>();

    storage.create_aggregate_function<MeanFunction>();

    storage.replace(User{1});
    storage.replace(User{2});
    storage.replace(User{3});
    REQUIRE(storage.count<User>() == 3);
    {
        REQUIRE(MeanFunction::objectsCount == 0);
        auto rows = storage.select(func<MeanFunction>(&User::id));
        REQUIRE(MeanFunction::objectsCount == 0);
        decltype(rows) expected;
        expected.push_back(2);
        REQUIRE(rows == expected);
    }
    storage.delete_aggregate_function<MeanFunction>();

    storage.create_scalar_function<FirstFunction>();
    {
        auto rows = storage.select(func<FirstFunction>("Vanotek", "Tinashe", "Pitbull"));
        decltype(rows) expected;
        expected.push_back("VTP");
        printArray(rows, "rows");
        printArray(expected, "expected");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 1);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Charli XCX", "Rita Ora"));
        decltype(rows) expected;
        expected.push_back("CR");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 2);
    }
    {
        auto rows = storage.select(func<FirstFunction>("Ted"));
        decltype(rows) expected;
        expected.push_back("T");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 3);
    }
    {
        auto rows = storage.select(func<FirstFunction>());
        decltype(rows) expected;
        expected.push_back("");
        REQUIRE(rows == expected);
        REQUIRE(FirstFunction::objectsCount == 0);
        REQUIRE(FirstFunction::callsCount == 4);
    }
    storage.delete_scalar_function<FirstFunction>();

    storage.create_aggregate_function<MultiSum>();
    {
        REQUIRE(MultiSum::objectsCount == 0);
        auto rows = storage.select(func<MultiSum>(&User::id, 5));
        decltype(rows) expected;
        expected.push_back(21);
        REQUIRE(rows == expected);
        REQUIRE(MultiSum::objectsCount == 0);
    }
    storage.delete_aggregate_function<MultiSum>();
}

TEST_CASE("InsertRange") {
    struct Object {
        int id;
        std::string name;
    };

    struct ObjectWithoutRowid {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "test_insert_range.sqlite",
        make_table("objects", make_column("id", &Object::id, primary_key()), make_column("name", &Object::name)),
        make_table("objects_without_rowid",
                   make_column("id", &ObjectWithoutRowid::id, primary_key()),
                   make_column("name", &ObjectWithoutRowid::name))
            .without_rowid());

    storage.sync_schema();
    storage.remove_all<Object>();
    storage.remove_all<ObjectWithoutRowid>();

    SECTION("straight") {
        std::vector<Object> objects = {100,
                                       Object{
                                           0,
                                           "Skillet",
                                       }};
        storage.insert_range(objects.begin(), objects.end());
        REQUIRE(storage.count<Object>() == 100);

        //  test empty container
        std::vector<Object> emptyVector;
        storage.insert_range(emptyVector.begin(), emptyVector.end());

        //  test insert_range without rowid
        std::vector<ObjectWithoutRowid> objectsWR = {ObjectWithoutRowid{10, "Life"}, ObjectWithoutRowid{20, "Death"}};
        REQUIRE(objectsWR.size() == 2);
        storage.insert_range(objectsWR.begin(), objectsWR.end());
        REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
        REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
    }
    SECTION("pointers") {
        std::vector<std::unique_ptr<Object>> objects;
        objects.reserve(100);
        for(auto i = 0; i < 100; ++i) {
            objects.push_back(std::make_unique<Object>(Object{0, "Skillet"}));
        }
        storage.insert_range<Object>(objects.begin(),
                                     objects.end(),
                                     [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                         return *pointer;
                                     });
        REQUIRE(storage.count<Object>() == 100);

        //  test empty container
        std::vector<std::unique_ptr<Object>> emptyVector;
        storage.insert_range<Object>(emptyVector.begin(),
                                     emptyVector.end(),
                                     [](const std::unique_ptr<Object> &pointer) -> const Object & {
                                         return *pointer;
                                     });

        //  test insert_range without rowid
        std::vector<std::unique_ptr<ObjectWithoutRowid>> objectsWR;
        objectsWR.push_back(std::make_unique<ObjectWithoutRowid>(ObjectWithoutRowid{10, "Life"}));
        objectsWR.push_back(std::make_unique<ObjectWithoutRowid>(ObjectWithoutRowid{20, "Death"}));

        REQUIRE(objectsWR.size() == 2);
        storage.insert_range<ObjectWithoutRowid>(
            objectsWR.begin(),
            objectsWR.end(),
            [](const std::unique_ptr<ObjectWithoutRowid> &pointer) -> const ObjectWithoutRowid & {
                return *pointer;
            });
        REQUIRE(storage.get<ObjectWithoutRowid>(10).name == "Life");
        REQUIRE(storage.get<ObjectWithoutRowid>(20).name == "Death");
    }
}
