#include <sqlite_orm/sqlite_orm.h>

#include <iostream>  //  std::cout, std::endl

using namespace sqlite_orm;

using std::cout;
using std::endl;

void testSubstr() {
    cout << __func__ << endl;
    
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
        assert(rows.size() == 1);
        assert(rows.front() == "substr");
    }
    {
        storage.insert(Test{"SQLite substr", 8});
        assert(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x));
        assert(rows.size() == 1);
        assert(rows.front() == "substr");
    }
    {
        auto rows = storage.select(substr("SQLite substr", 1, 6));
        assert(rows.size() == 1);
        assert(rows.front() == "SQLite");
    }
    {
        
        storage.remove_all<Test>();
        storage.insert(Test{"SQLite substr", 1, 6});
        assert(storage.count<Test>() == 1);
        auto rows = storage.select(substr(&Test::text, &Test::x, &Test::y));
        assert(rows.size() == 1);
        assert(rows.front() == "SQLite");
    }
}

void testZeroblob() {
    cout << __func__ << endl;
    
    struct Test {
        int value = 0;
    };
    
    auto storage = make_storage({},
                                make_table("test",
                                           make_column("value", &Test::value)));
    storage.sync_schema();
    
    {
        auto rows = storage.select(zeroblob(10));
        assert(rows.size() == 1);
        auto &row = rows.front();
        assert(row.size() == 10);
        std::vector<char> expectedValue(10);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        assert(row == expectedValue);
    }
    {
        storage.insert(Test{100});
        
        auto rows = storage.select(zeroblob(&Test::value));
        assert(rows.size() == 1);
        auto &row = rows.front();
        assert(row.size() == 100);
        std::vector<char> expectedValue(100);
        std::fill(expectedValue.begin(), expectedValue.end(), 0);
        assert(row == expectedValue);
    }
}

void testJulianday() {
    cout << __func__ << endl;
    
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
            assert(rows.size() == 1);
            assert((rows.front() - expected) < 0.001); //  too much precision
        }
        {
            storage.insert(Test{arg});
            auto rows = storage.select(julianday(&Test::text));
            assert(rows.size() == 1);
            assert((rows.front() - expected) < 0.001);
            storage.remove_all<Test>();
        }
    };
    singleTestCase("2016-10-18", 2457679.5);
    singleTestCase("2016-10-18 16:45", 2457680.19791667);
    singleTestCase("2016-10-18 16:45:30", 2457680.19826389);
}

void testDatetime() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(datetime("now"));
    assert(rows.size() == 1);
    assert(!rows.front().empty());
}

void testDate() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(date("now", "start of month", "+1 month", "-1 day"));
    assert(rows.size() == 1);
    assert(!rows.front().empty());
    
}

#if SQLITE_VERSION_NUMBER >= 3007016
void testChar() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(char_(67,72,65,82));
    assert(rows.size() == 1);
    assert(rows.front() == "CHAR");
}
#endif

void testRtrim() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(rtrim("ototo   "));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
    
    rows = storage.select(rtrim("ototo   ", " "));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
}

void testLtrim() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(ltrim("  ototo"));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
    
    rows = storage.select(ltrim("  ototo", " "));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
}

void testTrim() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(trim("   ototo   "));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
    
    rows = storage.select(trim("   ototo   ", " "));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
}

void testUpper() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(upper("ototo"));
    assert(rows.size() == 1);
    assert(rows.front() == "OTOTO");
}

void testLower() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(lower("OTOTO"));
    assert(rows.size() == 1);
    assert(rows.front() == "ototo");
}

void testLength() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(length("ototo"));
    assert(rows.size() == 1);
    assert(rows.front() == 5);
}

void testAbs() {
    cout << __func__ << endl;
    
    auto storage = make_storage({});
    auto rows = storage.select(sqlite_orm::abs(-10));
    assert(rows.size() == 1);
    assert(rows.front());
    assert(*rows.front() == 10);
}

int main() {
    testLength();
    
    testAbs();
    
    testLower();
    
    testUpper();
    
    testTrim();
    
    testLtrim();
    
    testRtrim();
#if SQLITE_VERSION_NUMBER >= 3007016
    testChar();
#endif
    
    testDate();
    
    testDatetime();
    
    testJulianday();
    
    testZeroblob();
    
    testSubstr();
    
    return 0;
}

