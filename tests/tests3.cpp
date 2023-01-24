#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("Multi order by") {
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

    storage.insert(Singer{0, "Alexandra Stan", "female"});
    storage.insert(Singer{0, "Inna", "female"});
    storage.insert(Singer{0, "Krewella", "female"});
    storage.insert(Singer{0, "Sting", "male"});
    storage.insert(Singer{0, "Lady Gaga", "female"});
    storage.insert(Singer{0, "Rameez", "male"});

    {
        //  test double ORDER BY
        auto singers = storage.get_all<Singer>(
            multi_order_by(order_by(&Singer::name).asc().collate_nocase(), order_by(&Singer::gender).desc()));
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

TEST_CASE("Issue 105") {
    struct Data {
        int str;
    };

    auto storage = make_storage("", make_table("data", make_column("str", &Data::str, primary_key())));

    storage.sync_schema();

    Data d{0};
    storage.insert(d);
}

TEST_CASE("Issue 87") {
    struct Data {
        uint8_t mDefault = 0; /**< 0=User or 1=Default*/
        uint8_t mAppLang = 0;  // en_GB
        uint8_t mContentLang1 = 0;  // de_DE
        uint8_t mContentLang2 = 0;  // en_GB
        uint8_t mContentLang3 = 0;
        uint8_t mContentLang4 = 0;
        uint8_t mContentLang5 = 0;
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
TEST_CASE("Wide string") {

    struct Alphabet {
        int id;
        std::wstring letters;
    };

    auto storage = make_storage("wideStrings.sqlite",
                                make_table("alphabets",
                                           make_column("id", &Alphabet::id, primary_key()),
                                           make_column("letters", &Alphabet::letters)));
    storage.sync_schema();
    storage.remove_all<Alphabet>();

    std::vector<std::wstring> expectedStrings = {
        L"ﻌﺾﺒﺑﺏﺖﺘﺗﺕﺚﺜﺛﺙﺞﺠﺟﺝﺢﺤﺣﺡﺦﺨﺧﺥﺪﺩﺬﺫﺮﺭﺰﺯﺲﺴﺳﺱﺶﺸﺷﺵﺺﺼﺻﺹﻀﺿﺽﻂﻄﻃﻁﻆﻈﻇﻅﻊﻋﻉﻎﻐﻏﻍﻒﻔﻓﻑﻖﻘﻗﻕﻚﻜﻛﻙﻞﻠﻟﻝﻢﻤﻣﻡﻦﻨﻧﻥﻪﻬﻫﻩﻮﻭﻲﻴﻳ"
        L"ﻱ",  //  arabic
        L"ԱաԲբԳգԴդԵեԶզԷէԸըԹթԺժԻիԼլԽխԾծԿկՀհՁձՂղՃճՄմՅյՆնՇշՈոՉչՊպՋջՌռՍսՎվՏտՐրՑցՒւՓփՔքՕօՖֆ",  //  armenian
        L"АаБбВвГгДдЕеЁёЖжЗзИиЙйКкЛлМмНнОоППРрСсТтУуФфХхЦцЧчШшЩщЪъЫыЬьЭэЮюЯя",  //  russian
        L"AaBbCcÇçDdEeFFGgĞğHhIıİiJjKkLlMmNnOoÖöPpRrSsŞşTtUuÜüVvYyZz",  //  turkish
    };
    for(auto& expectedString: expectedStrings) {
        auto id = storage.insert(Alphabet{0, expectedString});
        REQUIRE(storage.get<Alphabet>(id).letters == expectedString);
    }
}
#endif  //  SQLITE_ORM_OMITS_CODECVT
#ifdef SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("dbstat") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto storage =
        make_storage("dbstat.sqlite",
                     make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)),
                     make_dbstat_table());
    storage.sync_schema();

    storage.remove_all<User>();

    storage.replace(User{1, "Dua Lipa"});

    auto dbstatRows = storage.get_all<dbstat>();
    std::ignore = dbstatRows;
}
#endif  //  SQLITE_ENABLE_DBSTAT_VTAB
TEST_CASE("Busy timeout") {
    auto storage = make_storage("testBusyTimeout.sqlite");
    storage.busy_timeout(500);
}

TEST_CASE("Aggregate functions") {
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

    storage.replace(User{1, "Bebe Rexha", 28});
    storage.replace(User{2, "Rihanna", 29});
    storage.replace(User{3, "Cheryl Cole", 34});

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

TEST_CASE("Current timestamp") {
    auto storage = make_storage("");
    REQUIRE(storage.current_timestamp().size());

    storage.begin_transaction();
    REQUIRE(storage.current_timestamp().size());
    storage.commit();
}

TEST_CASE("Open forever") {
    struct User {
        int id;
        std::string name;
    };

    auto storage = make_storage(
        "open_forever.sqlite",
        make_table("users", make_column("id", &User::id, primary_key()), make_column("name", &User::name)));
    storage.sync_schema();

    storage.remove_all<User>();

    storage.open_forever();

    storage.insert(User{1, "Demi"});
    storage.insert(User{2, "Luis"});
    storage.insert(User{3, "Shakira"});

    storage.open_forever();

    REQUIRE(storage.count<User>() == 3);

    storage.begin_transaction();
    storage.insert(User{4, "Calvin"});
    storage.commit();

    REQUIRE(storage.count<User>() == 4);
}

//  appeared after #54
TEST_CASE("Blob") {
    struct BlobData {
        std::vector<char> data;
    };
    using byte = char;

    auto generateData = [](size_t size) -> byte* {
        auto data = (byte*)::malloc(size * sizeof(byte));
        for(int i = 0; i < static_cast<int>(size); ++i) {
            if((i + 1) % 10 == 0) {
                data[i] = 0;
            } else {
                data[i] = static_cast<byte>((rand() % 100) + 1);
            }
        }
        return data;
    };

    auto storage = make_storage("blob.db", make_table("blob", make_column("data", &BlobData::data)));
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
        REQUIRE(vd.size() == 1);
        auto& blob = vd.front();
        REQUIRE(blob.data.size() == size);
        REQUIRE(std::equal(data, data + size, blob.data.begin()));
    }

    //  read data with select (single column)
    {
        auto blobData = storage.select(&BlobData::data);
        REQUIRE(blobData.size() == 1);
        auto& blob = blobData.front();
        REQUIRE(blob.size() == size);
        REQUIRE(std::equal(data, data + size, blob.begin()));
    }

    //  read data with select (multi column)
    {
        auto blobData = storage.select(columns(&BlobData::data));
        REQUIRE(blobData.size() == 1);
        auto& blob = std::get<0>(blobData.front());
        REQUIRE(blob.size() == size);
        REQUIRE(std::equal(data, data + size, blob.begin()));
    }

    storage.insert(BlobData{});

    free(data);
}

/**
 *  Created by fixing https://github.com/fnc12/sqlite_orm/issues/42
 */
TEST_CASE("Escape chars") {
    struct Employee {
        int id;
        std::string name;
        int age;
        std::string address;
        double salary;
    };
    auto storage = make_storage("test_escape_chars.sqlite",
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
