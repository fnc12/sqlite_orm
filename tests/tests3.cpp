#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("Multi order by"){
    struct Singer {
        int id;
        std::string name;
        std::string gender;
    };
    
    auto storage = make_storage("",
                                make_table("singers",
                                           make_column("id", &Singer::id, primary_key()),
                                           make_column("name", &Singer::name, unique()),
                                           make_column("gender", &Singer::gender)));
    storage.sync_schema();
    
    storage.insert(Singer{ 0, "Alexandra Stan", "female" });
    storage.insert(Singer{ 0, "Inna", "female" });
    storage.insert(Singer{ 0, "Krewella", "female" });
    storage.insert(Singer{ 0, "Sting", "male" });
    storage.insert(Singer{ 0, "Lady Gaga", "female" });
    storage.insert(Singer{ 0, "Rameez", "male" });
    
    {
        //  test double ORDER BY
        auto singers = storage.get_all<Singer>(multi_order_by(order_by(&Singer::name).asc().collate_nocase(), order_by(&Singer::gender).desc()));
        auto expectedIds = {1, 2, 3, 5, 6, 4};
        REQUIRE(expectedIds.size() == singers.size());
        auto it = expectedIds.begin();
        for(size_t i = 0; i < singers.size(); ++i) {
            REQUIRE(*it == singers[i].id);
            ++it;
        }
    }
    
    //  test multi ORDER BY ith singl column with single ORDER BY
    {
        auto singers = storage.get_all<Singer>(order_by(&Singer::id).asc());
        auto singers2 = storage.get_all<Singer>(multi_order_by(order_by(&Singer::id).asc()));
        REQUIRE(singers.size() == singers2.size());
        for(size_t i = 0; i < singers.size(); ++i) {
            REQUIRE(singers[i].id == singers2[i].id);
        }
    }
    
}

TEST_CASE("Issue 105"){
    struct Data {
        int str;
    };
    
    auto storage = make_storage("",
                                make_table("data",
                                           make_column("str", &Data::str, primary_key())));
    
    storage.sync_schema();
    
    Data d{0};
    storage.insert(d);
}

TEST_CASE("Row id"){
    struct SimpleTable {
        std::string letter;
        std::string desc;
    };
    
    auto storage = make_storage("rowid.sqlite",
                                make_table("tbl1",
                                           make_column("letter", &SimpleTable::letter),
                                           make_column("desc", &SimpleTable::desc)));
    storage.sync_schema();
    storage.remove_all<SimpleTable>();
    
    storage.insert(SimpleTable{"A", "first letter"});
    storage.insert(SimpleTable{"B", "second letter"});
    storage.insert(SimpleTable{"C", "third letter"});
    
    auto rows = storage.select(columns(rowid(),
                                       oid(),
                                       _rowid_(),
                                       rowid<SimpleTable>(),
                                       oid<SimpleTable>(),
                                       _rowid_<SimpleTable>(),
                                       &SimpleTable::letter,
                                       &SimpleTable::desc));
    for(size_t i = 0; i < rows.size(); ++i) {
        auto &row = rows[i];
        REQUIRE(std::get<0>(row) == std::get<1>(row));
        REQUIRE(std::get<1>(row) == std::get<2>(row));
        REQUIRE(std::get<2>(row) == static_cast<int>(i + 1));
        REQUIRE(std::get<2>(row) == std::get<3>(row));
        REQUIRE(std::get<3>(row) == std::get<4>(row));
        REQUIRE(std::get<4>(row) == std::get<5>(row));
    }
}

TEST_CASE("Issue 87"){
    struct Data {
        uint8_t  mDefault; /**< 0=User or 1=Default*/
        uint8_t     mAppLang; //en_GB
        uint8_t     mContentLang1; //de_DE
        uint8_t     mContentLang2; //en_GB
        uint8_t     mContentLang3;
        uint8_t     mContentLang4;
        uint8_t     mContentLang5;
    };
    
    Data data;
    
    auto storage = make_storage("",
                                make_table("data",
                                           make_column("IsDef", &Data::mDefault, primary_key()),
                                           make_column("AL", &Data::mAppLang),
                                           make_column("CL1", &Data::mContentLang1),
                                           make_column("CL2", &Data::mContentLang2),
                                           make_column("CL3", &Data::mContentLang3),
                                           make_column("CL4", &Data::mContentLang4),
                                           make_column("CL5", &Data::mContentLang5)));
    storage.sync_schema();
    
    storage.update_all(set(c(&Data::mContentLang1) = data.mContentLang1,
                           c(&Data::mContentLang2) = data.mContentLang2,
                           c(&Data::mContentLang3) = data.mContentLang3,
                           c(&Data::mContentLang4) = data.mContentLang4,
                           c(&Data::mContentLang5) = data.mContentLang5),
                       where(c(&Data::mDefault) == data.mDefault));
}

#ifndef SQLITE_ORM_OMITS_CODECVT
TEST_CASE("Wide string"){
    
    struct Alphabet {
        int id;
        std::wstring letters;
    };
    
    auto storage = make_storage("wideStrings.sqlite",
                                make_table("alphabets",
                                           make_column("id",
                                                       &Alphabet::id,
                                                       primary_key()),
                                           make_column("letters",
                                                       &Alphabet::letters)));
    storage.sync_schema();
    storage.remove_all<Alphabet>();
    
    std::vector<std::wstring> expectedStrings = {
        L"ﻌﺾﺒﺑﺏﺖﺘﺗﺕﺚﺜﺛﺙﺞﺠﺟﺝﺢﺤﺣﺡﺦﺨﺧﺥﺪﺩﺬﺫﺮﺭﺰﺯﺲﺴﺳﺱﺶﺸﺷﺵﺺﺼﺻﺹﻀﺿﺽﻂﻄﻃﻁﻆﻈﻇﻅﻊﻋﻉﻎﻐﻏﻍﻒﻔﻓﻑﻖﻘﻗﻕﻚﻜﻛﻙﻞﻠﻟﻝﻢﻤﻣﻡﻦﻨﻧﻥﻪﻬﻫﻩﻮﻭﻲﻴﻳﻱ",   //  arabic
        L"ԱաԲբԳգԴդԵեԶզԷէԸըԹթԺժԻիԼլԽխԾծԿկՀհՁձՂղՃճՄմՅյՆնՇշՈոՉչՊպՋջՌռՍսՎվՏտՐրՑցՒւՓփՔքՕօՖֆ",    //  armenian
        L"АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоППРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯя",  //  russian
        L"AaBbCcÇçDdEeFFGgĞğHhIıİiJjKkLlMmNnOoÖöPpRrSsŞşTtUuÜüVvYyZz",  //  turkish
    };
    for(auto &expectedString : expectedStrings) {
        auto id = storage.insert(Alphabet{0, expectedString});
        REQUIRE(storage.get<Alphabet>(id).letters == expectedString);
    }
    
}
#endif  //  SQLITE_ORM_OMITS_CODECVT

TEST_CASE("Issue 86"){
    struct Data
    {
        uint8_t mUsed=0;
        int mId = 0;
        int mCountryId = 0;
        int mLangId = 0;
        std::string mName;
    };
    
    auto storage = make_storage("",
                                make_table("cities",
                                           make_column("U", &Data::mUsed),
                                           make_column("Id", &Data::mId),
                                           make_column("CntId", &Data::mCountryId),
                                           make_column("LId", &Data::mLangId),
                                           make_column("N", &Data::mName)));
    storage.sync_schema();
    std::string CurrCity = "ototo";
    auto vms = storage.select(columns(&Data::mId, &Data::mLangId),
                              where(like(&Data::mName, CurrCity)));
}

TEST_CASE("Busy timeout"){
    auto storage = make_storage("testBusyTimeout.sqlite");
    storage.busy_timeout(500);
}

TEST_CASE("Aggregate functions"){
    struct User {
        int id;
        std::string name;
        int age;
        
        void setId(int newValue) {
            this->id = newValue;
        }
        
        const int& getId() const {
            return this->id;
        }
        
        void setName(std::string newValue) {
            this->name = newValue;
        }
        
        const std::string& getName() const {
            return this->name;
        }
        
        void setAge(int newValue) {
            this->age = newValue;
        }
        
        const int& getAge() const {
            return this->age;
        }
    };
    
    auto storage = make_storage("test_aggregate.sqlite",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name),
                                           make_column("age", &User::age)));
    auto storage2 = make_storage("test_aggregate.sqlite",
                                 make_table("users",
                                            make_column("id", &User::getId, &User::setId, primary_key()),
                                            make_column("name", &User::getName, &User::setName),
                                            make_column("age", &User::getAge, &User::setAge)));
    storage.sync_schema();
    storage.remove_all<User>();
    
    storage.replace(User{ 1, "Bebe Rexha", 28});
    storage.replace(User{ 2, "Rihanna", 29 });
    storage.replace(User{ 3, "Cheryl Cole", 34 });
    
    auto avgId = storage.avg(&User::id);
    REQUIRE(avgId == 2);
    
    auto avgId2 = storage2.avg(&User::getId);
    REQUIRE(avgId2 == avgId);
    
    auto avgId3 = storage2.avg(&User::setId);
    REQUIRE(avgId3 == avgId2);
    
    auto avgRaw = storage.select(avg(&User::id)).front();
    REQUIRE(avgRaw == avgId);
    
    auto distinctAvg = storage.select(distinct(avg(&User::id))).front();
    REQUIRE(distinctAvg == avgId);
    
    auto allAvg = storage.select(all(avg(&User::id))).front();
    REQUIRE(allAvg == avgId);
    
    auto avgRaw2 = storage2.select(avg(&User::getId)).front();
    REQUIRE(avgRaw2 == avgId);
    
    auto avgRaw3 = storage2.select(avg(&User::setId)).front();
    REQUIRE(avgRaw3 == avgRaw2);
    
    auto distinctAvg2 = storage2.select(distinct(avg(&User::setId))).front();
    REQUIRE(distinctAvg2 == avgId);
    
    auto distinctAvg3 = storage2.select(distinct(avg(&User::getId))).front();
    REQUIRE(distinctAvg3 == distinctAvg2);
    
    auto allAvg2 = storage2.select(all(avg(&User::getId))).front();
    REQUIRE(allAvg2 == avgId);
    
    auto allAvg3 = storage2.select(all(avg(&User::setId))).front();
    REQUIRE(allAvg3 == allAvg2);
    
    //  next we test that all aggregate functions support arguments bindings.
    //  This is why id = 1 condition is important here
    {
        auto avg1 = storage.avg(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(avg1 == 1);
    }
    {
        auto count1 = storage.count(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(count1 == 1);
    }
    {
        auto max1 = storage.max(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(max1);
        REQUIRE(*max1 == 1);
    }
    {
        auto min1 = storage.min(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(min1);
        REQUIRE(*min1 == 1);
    }
    {
        auto total1 = storage.total(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(total1 == 1);
    }
    {
        auto sum1 = storage.sum(&User::id, where(is_equal(&User::id, 1)));
        REQUIRE(sum1);
        REQUIRE(*sum1 == 1);
    }
    {
        auto groupConcat = storage.group_concat(&User::name, where(is_equal(&User::id, 1)));
        REQUIRE(groupConcat == "Bebe Rexha");
    }
}

TEST_CASE("Current timestamp"){
    auto storage = make_storage("");
    REQUIRE(storage.current_timestamp().size());
    
    storage.begin_transaction();
    REQUIRE(storage.current_timestamp().size());
    storage.commit();
}

TEST_CASE("Open forever"){
    struct User {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("open_forever.sqlite",
                                make_table("users",
                                           make_column("id", &User::id, primary_key()),
                                           make_column("name", &User::name)));
    storage.sync_schema();
    
    storage.remove_all<User>();
    
    storage.open_forever();
    
    storage.insert(User{ 1, "Demi" });
    storage.insert(User{ 2, "Luis" });
    storage.insert(User{ 3, "Shakira" });
    
    storage.open_forever();
    
    REQUIRE(storage.count<User>() == 3);
    
    storage.begin_transaction();
    storage.insert(User{ 4, "Calvin" });
    storage.commit();
    
    REQUIRE(storage.count<User>() == 4);
}

//  appeared after #55
TEST_CASE("Default value"){
    struct User {
        int userId;
        std::string name;
        int age;
        std::string email;
    };
    
    auto storage1 = make_storage("test_db.sqlite",
                                 make_table("User",
                                            make_column("Id", &User::userId, primary_key()),
                                            make_column("Name", &User::name),
                                            make_column("Age", &User::age)));
    storage1.sync_schema();
    storage1.remove_all<User>();
    
    auto emailColumn = make_column("Email", &User::email, default_value("example@email.com"));
    
    auto storage2 = make_storage("test_db.sqlite",
                                 make_table("User",
                                            make_column("Id", &User::userId, primary_key()),
                                            make_column("Name", &User::name),
                                            make_column("Age", &User::age),
                                            emailColumn));
    storage2.sync_schema();
    storage2.insert(User{0, "Tom", 15, ""});
    
    auto emailDefault = emailColumn.default_value();
    REQUIRE(emailDefault);
    REQUIRE(*emailDefault == "example@email.com");
}

//  appeared after #54
TEST_CASE("Blob"){
    struct BlobData {
        std::vector<char> data;
    };
    using byte = char;
    
    auto generateData = [](size_t size) -> byte* {
        auto data = (byte*)::malloc(size * sizeof(byte));
        for (int i = 0; i < static_cast<int>(size); ++i) {
            if ((i+1) % 10 == 0) {
                data[i] = 0;
            } else {
                data[i] = static_cast<byte>((rand() % 100) + 1);
            }
        }
        return data;
    };
    
    auto storage = make_storage("blob.db",
                                make_table("blob",
                                           make_column("data", &BlobData::data)));
    storage.sync_schema();
    storage.remove_all<BlobData>();
    
    size_t size = 100;
    auto data = generateData(size);
    
    //  write data
    BlobData d;
    std::vector<char> v(data, data + size);
    d.data = v;
    storage.insert(d);
    
    //  read data with get_all
    {
        auto vd = storage.get_all<BlobData>();
        assert(vd.size() == 1);
        auto &blob = vd.front();
        REQUIRE(blob.data.size() == size);
        REQUIRE(std::equal(data, data + size, blob.data.begin()));
    }
    
    //  read data with select (single column)
    {
        auto blobData = storage.select(&BlobData::data);
        assert(blobData.size() == 1);
        auto &blob = blobData.front();
        REQUIRE(blob.size() == size);
        REQUIRE(std::equal(data, data + size, blob.begin()));
    }
    
    //  read data with select (multi column)
    {
        auto blobData = storage.select(columns(&BlobData::data));
        REQUIRE(blobData.size() == 1);
        auto &blob = std::get<0>(blobData.front());
        REQUIRE(blob.size() == size);
        REQUIRE(std::equal(data,
                           data + size,
                           blob.begin()));
    }
    
    storage.insert(BlobData{});
    
    free(data);
}

//  appeared after #57
TEST_CASE("Foreign key 2"){
    class test1 {
    public:
        // Constructors
        test1() {};
        
        // Variables
        int id;
        std::string val1;
        std::string val2;
    };
    
    class test2 {
    public:
        // Constructors
        test2() {};
        
        // Variables
        int id;
        int fk_id;
        std::string val1;
        std::string val2;
    };
    
    auto table1 = make_table("test_1",
                             make_column("id", &test1::id, primary_key()),
                             make_column("val1", &test1::val1),
                             make_column("val2", &test1::val2));
    
    auto table2 = make_table("test_2",
                             make_column("id", &test2::id, primary_key()),
                             make_column("fk_id", &test2::fk_id),
                             make_column("val1", &test2::val1),
                             make_column("val2", &test2::val2),
                             foreign_key(&test2::fk_id).references(&test1::id));
    
    auto storage = make_storage("test.sqlite",
                                table1,
                                table2);
    
    storage.sync_schema();
    
    
    test1 t1;
    t1.val1 = "test";
    t1.val2 = "test";
    storage.insert(t1);
    
    test1 t1_copy;
    t1_copy.val1 = "test";
    t1_copy.val2 = "test";
    storage.insert(t1_copy);
    
    test2 t2;
    t2.fk_id = 1;
    t2.val1 = "test";
    t2.val2 = "test";
    storage.insert(t2);
    
    t2.fk_id = 2;
    
    storage.update(t2);
}

TEST_CASE("Foreign key"){
    
    struct Location {
        int id;
        std::string place;
        std::string country;
        std::string city;
        int distance;
        
    };
    
    struct Visit {
        int id;
        std::unique_ptr<int> location;
        std::unique_ptr<int> user;
        int visited_at;
        uint8_t mark;
    };
    
    //  this case didn't compile on linux until `typedef constraints_type` was added to `foreign_key_t`
    auto storage = make_storage("test_fk.sqlite",
                                make_table("location",
                                           make_column("id", &Location::id, primary_key()),
                                           make_column("place", &Location::place),
                                           make_column("country", &Location::country),
                                           make_column("city", &Location::city),
                                           make_column("distance", &Location::distance)),
                                make_table("visit",
                                           make_column("id", &Visit::id, primary_key()),
                                           make_column("location", &Visit::location),
                                           make_column("user", &Visit::user),
                                           make_column("visited_at", &Visit::visited_at),
                                           make_column("mark", &Visit::mark),
                                           foreign_key(&Visit::location).references(&Location::id)));
    storage.sync_schema();
    
    int fromDate = int(time(nullptr));
    int toDate = int(time(nullptr));
    int toDistance = 100;
    auto id = 10;
    storage.select(columns(&Visit::mark, &Visit::visited_at, &Location::place),
                   inner_join<Location>(on(is_equal(&Visit::location, &Location::id))),
                   where(is_equal(&Visit::user, id) and
                         greater_than(&Visit::visited_at, fromDate) and
                         lesser_than(&Visit::visited_at, toDate) and
                         lesser_than(&Location::distance, toDistance)),
                   order_by(&Visit::visited_at));
}

/**
 *  Created by fixing https://github.com/fnc12/sqlite_orm/issues/42
 */
TEST_CASE("Escape chars"){
    struct Employee {
        int id;
        std::string name;
        int age;
        std::string address;
        double salary;
    };
    auto storage =  make_storage("test_escape_chars.sqlite",
                                 make_table("COMPANY",
                                            make_column("INDEX", &Employee::id, primary_key()),
                                            make_column("NAME", &Employee::name),
                                            make_column("AGE", &Employee::age),
                                            make_column("ADDRESS", &Employee::address),
                                            make_column("SALARY", &Employee::salary)));
    storage.sync_schema();
    storage.remove_all<Employee>();
    
    storage.insert(Employee{
        0,
        "Paul'l",
        20,
        "Sacramento 20",
        40000,
    });
    
    auto paulL = storage.get_all<Employee>(where(is_equal(&Employee::name, "Paul'l")));
    
    storage.replace(Employee{
        10,
        "Selena",
        24,
        "Florida",
        500000,
    });
    
    auto selena = storage.get<Employee>(10);
    auto selenaMaybe = storage.get_no_throw<Employee>(10);
    selena.name = "Gomez";
    storage.update(selena);
    storage.remove<Employee>(10);
}

TEST_CASE("Transaction guard"){
    struct Object {
        int id;
        std::string name;
    };
    
    auto storage = make_storage("test_transaction_guard.sqlite",
                                make_table("objects",
                                           make_column("id", &Object::id, primary_key()),
                                           make_column("name", &Object::name)));
    
    storage.sync_schema();
    storage.remove_all<Object>();
    
    storage.insert(Object{0, "Jack"});
    
    //  insert, call make a storage to cakk an exception and check that rollback was fired
    auto countBefore = storage.count<Object>();
    try{
        auto guard = storage.transaction_guard();
        
        storage.insert(Object{0, "John"});
        
        storage.get<Object>(-1);
        
        REQUIRE(false);
    }catch(...){
        auto countNow = storage.count<Object>();
        
        REQUIRE(countBefore == countNow);
    }
    
    //  check that one can call other transaction functions without exceptions
    storage.transaction([&]{return false;});
    
    //  commit explicitly and check that after exception data was saved
    countBefore = storage.count<Object>();
    try{
        auto guard = storage.transaction_guard();
        storage.insert(Object{0, "John"});
        guard.commit();
        storage.get<Object>(-1);
        REQUIRE(false);
    }catch(...){
        auto countNow = storage.count<Object>();
        
        REQUIRE(countNow == countBefore + 1);
    }
    
    //  rollback explicitly
    countBefore = storage.count<Object>();
    try{
        auto guard = storage.transaction_guard();
        storage.insert(Object{0, "Michael"});
        guard.rollback();
        storage.get<Object>(-1);
        REQUIRE(false);
    }catch(...){
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore);
    }
    
    //  commit on exception
    countBefore = storage.count<Object>();
    try{
        auto guard = storage.transaction_guard();
        guard.commit_on_destroy = true;
        storage.insert(Object{0, "Michael"});
        storage.get<Object>(-1);
        REQUIRE(false);
    }catch(...){
        auto countNow = storage.count<Object>();
        REQUIRE(countNow == countBefore + 1);
    }
    
    //  work witout exception
    countBefore = storage.count<Object>();
    try{
        auto guard = storage.transaction_guard();
        guard.commit_on_destroy = true;
        storage.insert(Object{0, "Lincoln"});
        
    }catch(...){
        throw std::runtime_error("Must not fire");
    }
    auto countNow = storage.count<Object>();
    REQUIRE(countNow == countBefore + 1);
}
