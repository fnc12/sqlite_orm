#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch.hpp>

using namespace sqlite_orm;

TEST_CASE("substr"){
    struct Test {
        std::string text;
        int x = 0;
        int y = 0;
    };
    auto storage = make_storage({},
                                make_table("test",
                                           make_column("text", &Test::text),
                                           make_column("x", &Test::x),
                                           make_column("y", &Test::y)));
    storage.sync_schema();
    
    {
        auto rows = storage.select(substr("SQLite substr", 8));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        storage.insert(Test{"SQLite substr", 8});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "substr");
    }
    {
        auto rows = storage.select(substr("SQLite substr", 1, 6));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
    {
        
        storage.remove_all<Test>();
        storage.insert(Test{"SQLite substr", 1, 6});
        REQUIRE(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x, &Test::y));
        REQUIRE(rows.size() == 1);
        REQUIRE(rows.front() == "SQLite");
    }
}

TEST_CASE("zeroblob"){
    struct Test {
        int value = 0;
    };
    
    auto storage = make_storage({},
                                make_table("test",
                                           make_column("value", &Test::value)));
    storage.sync_schema();
    
    {
        auto rows = storage.select(zeroblob(10));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 10);
        std::vector<char> expectedValue(10);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
    {
        storage.insert(Test{100});
        
        auto rows = storage.select(zeroblob(&Test::value));
        REQUIRE(rows.size() == 1);
        auto &row = rows.front();
        REQUIRE(row.size() == 100);
        std::vector<char> expectedValue(100);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        REQUIRE(row == expectedValue);
    }
}

TEST_CASE("julianday"){
    struct Test {
        std::string text;
    };
    
    auto storage = make_storage({},
                                make_table("test",
                                           make_column("text", &Test::text)));
    storage.sync_schema();
    auto singleTestCase = [&storage](const std::string &arg, double expected){
        {
            auto rows = storage.select(julianday(arg));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001); //  too much precision
        }
        {
            storage.insert(Test{arg});
            auto rows = storage.select(julianday(&Test::text));
            REQUIRE(rows.size() == 1);
            REQUIRE((rows.front() - expected) < 0.001);
            storage.remove_all<Test>();
        }
    };
    singleTestCase("2016-10-18", 2457679.5);
    singleTestCase("2016-10-18 16:45", 2457680.19791667);
    singleTestCase("2016-10-18 16:45:30", 2457680.19826389);
}

TEST_CASE("datetime"){
    auto storage = make_storage({});
    auto rows = storage.select(datetime("now"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

TEST_CASE("date"){
    auto storage = make_storage({});
    auto rows = storage.select(date("now", "start of month", "+1 month", "-1 day"));
    REQUIRE(rows.size() == 1);
    REQUIRE(!rows.front().empty());
}

#if SQLITE_VERSION_NUMBER >= 3007016
TEST_CASE("char"){
    auto storage = make_storage({});
    auto rows = storage.select(char_(67,72,65,82));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "CHAR");
}
#endif

TEST_CASE("rtrim"){
    auto storage = make_storage({});
    auto rows = storage.select(rtrim("ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
    
    rows = storage.select(rtrim("ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("ltrim"){
    auto storage = make_storage({});
    auto rows = storage.select(ltrim("  ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
    
    rows = storage.select(ltrim("  ototo", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("trim"){
    auto storage = make_storage({});
    auto rows = storage.select(trim("   ototo   "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
    
    rows = storage.select(trim("   ototo   ", " "));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("upper"){
    auto storage = make_storage({});
    auto rows = storage.select(upper("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "OTOTO");
}

TEST_CASE("lower"){
    auto storage = make_storage({});
    auto rows = storage.select(lower("OTOTO"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == "ototo");
}

TEST_CASE("length"){
    auto storage = make_storage({});
    auto rows = storage.select(length("ototo"));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 5);
}

TEST_CASE("abs"){
    auto storage = make_storage({});
    auto rows = storage.select(sqlite_orm::abs(-10));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front());
    REQUIRE(*rows.front() == 10);
}

